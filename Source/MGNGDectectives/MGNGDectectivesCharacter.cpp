// Copyright Epic Games, Inc. All Rights Reserved.

#include "MGNGDectectivesCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "ItemActor.h"
#include "Components/ArrowComponent.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputSubsystems.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"


//////////////////////////////////////////////////////////////////////////
// AMGNGDectectivesCharacter

AMGNGDectectivesCharacter::AMGNGDectectivesCharacter() :
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	DecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("ObjetivoGranada"));
	DecalComponent->SetupAttachment(RootComponent);
	DecalComponent->SetWorldRotation(FRotator(0.0f,90.0f,0.0f));
	
	ArrowDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowDirection"));
	ArrowDirection->SetupAttachment(RootComponent);
	
	isRagdoll = false;
	LanzadoGranada = false;
	StartCount = false;

	counter = 0;

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if(OnlineSubsystem)
	{
		OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();

		if(GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				10.f,
				FColor::Purple,
				FString::Printf(TEXT("Found Subsystem: %s"), *OnlineSubsystem->GetSubsystemName().ToString())
			);
		}
	}
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AMGNGDectectivesCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AMGNGDectectivesCharacter::CreateGameSession()
{
	if (!OnlineSessionInterface.IsValid())
	{
		return;
	}

	auto ExistingSession = OnlineSessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		OnlineSessionInterface->DestroySession(NAME_GameSession);
	}

	//Add the Dalegate on DelegateList
	OnlineSessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	//Create Session
	TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());
	
	SessionSettings->bIsLANMatch = false;
	SessionSettings->NumPublicConnections = 4;
	SessionSettings->bAllowJoinInProgress = true;
	SessionSettings->bAllowJoinViaPresence = true;
	SessionSettings->bShouldAdvertise = true;
	SessionSettings->bUsesPresence = true;
	SessionSettings->bUseLobbiesIfAvailable = true;
	SessionSettings->Set(FName("MatchType"), FString("FreeForAll"), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	OnlineSessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *SessionSettings);
}

void AMGNGDectectivesCharacter::JoinGameSession()
{
	if(!OnlineSessionInterface)
	{
		return;
	}

	OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->MaxSearchResults = 10000;
	SessionSearch->bIsLanQuery = false;
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	OnlineSessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef());
}

void AMGNGDectectivesCharacter::OnCreateSessionComplete(FName SessionName, bool bWasSuccess)
{
	if (bWasSuccess)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Blue,
				FString::Printf(TEXT("Created Session %s"),*SessionName.ToString())
			);
		}

		UWorld* World = GetWorld();
		if(World)
		{
			World->ServerTravel(FString("/Game/ThirdPerson/Maps/Lobby?listen"));
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Blue,
			FString::Printf(TEXT("Created Session Failed"))
		);
	}
}

void AMGNGDectectivesCharacter::OnFindSessionComplete(bool bWasSuccessful)
{
	if (!OnlineSessionInterface)
	{
		return;
	}

	
	for(auto Result : SessionSearch->SearchResults)
	{
		FString Id = Result.GetSessionIdStr();
		FString User = Result.Session.OwningUserName;

		FString MatchType;
		Result.Session.SessionSettings.Get(FName("MatchType"), MatchType);

		//Debug
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Orange,
				FString::Printf(TEXT("Id: %s, User: %s"), *Id, *User)
			);
		}

		if(MatchType == FString("FreeForAll"))
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					15.f,
					FColor::Orange,
					FString::Printf(TEXT("Joining MatchType %s"), *MatchType)
				);
			}

			OnlineSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

			const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
			OnlineSessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, Result);
		}
	}
}

void AMGNGDectectivesCharacter::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (!OnlineSessionInterface.IsValid())
	{
		return;
	}

	FString Address;

	if(OnlineSessionInterface->GetResolvedConnectString(NAME_GameSession, Address))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Cyan,
				FString::Printf(TEXT("Connect to: %s"), *Address)
			);
			
		}

		APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
		if(PlayerController)
		{
			PlayerController->ClientTravel(Address, TRAVEL_Absolute);
		}
	}
}

void AMGNGDectectivesCharacter::Tick(float DeltaSeconds)
{
	if(isRagdoll)
	{
		const FVector NewLocation(
			GetMesh()->GetSocketLocation("spy_bones").X,
			GetMesh()->GetSocketLocation("spy_bones").Y,
			GetMesh()->GetSocketLocation("spy_bones").Z + 90
		);
		GetCapsuleComponent()->SetWorldLocation(NewLocation);
		tieso = true;
	}
	
	DecalComponent->SetVisibility(LanzadoGranada);
	
	if(LanzadoGranada)
	{
		MyRotator = GetControlRotation();
		ForwardVector = MyRotator.Vector();
		StartLocation = ArrowDirection->GetComponentLocation();
		LaunchVelocity = ForwardVector * 2000.0f;
		bTraceComplex = false;
		CollisionChannel = ECC_WorldDynamic;
		EDrawDebugTrace::Type DrawDebugType = EDrawDebugTrace::None;
		Params.bTraceComplex = bTraceComplex;
		UGameplayStatics::Blueprint_PredictProjectilePath_ByTraceChannel(
			GetWorld(),
			HitResults,
			OutPathPositions,
			OutLastTraceDestinations,
			StartLocation,
			LaunchVelocity,
			true,
			20.0f,
			CollisionChannel,
			Params.bTraceComplex,
			ActorsToIgnore,
			DrawDebugType,
			0.0f,
			15.0f,
			2.0f,
			0.0f
		);
		DecalComponent->SetWorldLocationAndRotation(HitResults.ImpactPoint, FQuat::MakeFromEuler(HitResults.ImpactNormal));
	}

	if(StartCount)
	{
		counter += DeltaSeconds;
		if(counter >= 0.5f && canSoot)
		{
			canSoot = false;
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = GetInstigator();
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			// Set the initial location and rotation for the new actor
			FVector SpawnLocation = GetActorLocation() + FVector(100.f, 0.f, 0.f);
			FRotator SpawnRotation = GetActorRotation();
			// Spawn the new actor
			UObject* SpawnActor = Cast<UObject>(StaticLoadObject(UObject::StaticClass(), NULL, TEXT("/Game/BP_Granade.BP_Granade")));
			UBlueprint* GeneratedBP = Cast<UBlueprint>(SpawnActor);
			GetWorld()->SpawnActor<AActor>(GeneratedBP->GeneratedClass, ArrowDirection->GetComponentLocation(), GetControlRotation(), SpawnParams);
		
		}
		else if(counter >= 2.0f)
		{
			granadeOpacity = 1.0;
			StartCount = false;
			counter = 0;
			canSoot = true;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AMGNGDectectivesCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		//Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMGNGDectectivesCharacter::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMGNGDectectivesCharacter::Look);

		//Throw
		EnhancedInputComponent->BindAction(ThrowAction, ETriggerEvent::Started, this, &AMGNGDectectivesCharacter::ThrowStart);
		EnhancedInputComponent->BindAction(ThrowReleased, ETriggerEvent::Triggered, this, &AMGNGDectectivesCharacter::ThrowRelease);

		//Pickup
		EnhancedInputComponent->BindAction(PickAction, ETriggerEvent::Started, this, &AMGNGDectectivesCharacter::PickUp);
		
	}
}

void AMGNGDectectivesCharacter::ThrowStart()
{
	if(!isRagdoll && canSoot)
		LanzadoGranada = true;
}

void AMGNGDectectivesCharacter::ThrowRelease()
{
	if(!isRagdoll && canSoot)
	{
		LanzadoGranada = false;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		StartCount = true;
		granadeOpacity = 0.2;
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(ShootAnimation, 2.0f);
			// Set the spawn parameters for the new actor
			/*FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = GetInstigator();
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			// Set the initial location and rotation for the new actor
			FVector SpawnLocation = GetActorLocation() + FVector(100.f, 0.f, 0.f);
			FRotator SpawnRotation = GetActorRotation();
			// Spawn the new actor
			UObject* SpawnActor = Cast<UObject>(StaticLoadObject(UObject::StaticClass(), NULL, TEXT("/Game/BP_Granade.BP_Granade")));
			UBlueprint* GeneratedBP = Cast<UBlueprint>(SpawnActor);
			GetWorld()->SpawnActor<AActor>(GeneratedBP->GeneratedClass, ArrowDirection->GetComponentLocation(), GetControlRotation(), SpawnParams);*/
		}
	}
}

void AMGNGDectectivesCharacter::Die(const FInputActionValue& Value)
{
	if(Controller != nullptr)
	{
		LanzadoGranada = false;
		// Set the spawn parameters for the new actor
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = GetInstigator();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		// Spawn the new actor
		UObject* SpawnActor = Cast<UObject>(StaticLoadObject(UObject::StaticClass(), NULL, TEXT("/Game/BP_Granade.BP_Granade")));
		UBlueprint* GeneratedBP = Cast<UBlueprint>(SpawnActor);
		GetWorld()->SpawnActor<AActor>(GeneratedBP->GeneratedClass, ArrowDirection->GetComponentLocation(), GetControlRotation(), SpawnParams);
	}
}

void AMGNGDectectivesCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AMGNGDectectivesCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AMGNGDectectivesCharacter::PrintOnDebug(FString TextToDisplay)
{
	GEngine->AddOnScreenDebugMessage(
		-1,
		1.0f,
		FColor::Red,
		TextToDisplay
	);
}

void AMGNGDectectivesCharacter::PickUp()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance != nullptr && canPick)
	{
		AnimInstance->Montage_Play(PickAnimation, 2.0f);
		Piece++;
		//canPick = false;
		itemClass->Destroy();
		itemClass = nullptr;
		canPick=false;
	}
}

float AMGNGDectectivesCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if(DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
	{
		GetMesh()->SetAllBodiesBelowSimulatePhysics("spy_bones", true);
		isRagdoll = true;
	}
	return 0;
}
