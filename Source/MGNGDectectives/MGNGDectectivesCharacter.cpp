// Copyright Epic Games, Inc. All Rights Reserved.

#include "MGNGDectectivesCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "Granade.h"
#include "EnhancedInputSubsystems.h"
#include "Components/ArrowComponent.h"
#include <chrono>
#include <thread>
#include "Kismet/GameplayStatics.h"


//////////////////////////////////////////////////////////////////////////
// AMGNGDectectivesCharacter

AMGNGDectectivesCharacter::AMGNGDectectivesCharacter()
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
	counter = 0;
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

void AMGNGDectectivesCharacter::Tick(float DeltaSeconds)
{
	if(isRagdoll)
	{
		const FVector NewLocation(
			GetMesh()->GetSocketLocation("pelvis").X,
			GetMesh()->GetSocketLocation("pelvis").Y,
			GetMesh()->GetSocketLocation("pelvis").Z + 90
		);
		GetCapsuleComponent()->SetWorldLocation(NewLocation);
	}
	/*DecalComponent->SetVisibility(LanzadoGranada);
	
	if(LanzadoGranada)
	{
		FHitResult HitResults;
		TArray<FVector> OutPathPositions;
		FVector OutLastTraceDestinations;

		TArray<AActor*> ActorsToIgnore;
		const FRotator MyRotator = GetControlRotation();
		const FVector ForwardVector = MyRotator.Vector();
		FVector StartLocation = /*Arrow->GetComponentLocation(); GetActorLocation();
		FVector LaunchVelocity = ForwardVector * Impulso;

		bool bTraceComplex = false;
		TEnumAsByte<ECollisionChannel> CollisionChannel = ECC_WorldDynamic;
		EDrawDebugTrace::Type DrawDebugType = EDrawDebugTrace::ForOneFrame;
		FCollisionQueryParams Params;
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
			false,
			ActorsToIgnore,
			DrawDebugType,
			0.0f,
			15.0f,
			2.0f,
			0.0f
		);
	}*/
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

		//Die
		EnhancedInputComponent->BindAction(DieAction, ETriggerEvent::Completed, this, &AMGNGDectectivesCharacter::Die);

		//Throw
		EnhancedInputComponent->BindAction(ThrowAction, ETriggerEvent::Started, this, &AMGNGDectectivesCharacter::ThrowStart);
		EnhancedInputComponent->BindAction(ThrowAction, ETriggerEvent::Ongoing, this, &AMGNGDectectivesCharacter::ThrowOnGoing);
		EnhancedInputComponent->BindAction(ThrowAction, ETriggerEvent::Canceled, this, &AMGNGDectectivesCharacter::ThrowRelease);
	}

}

void AMGNGDectectivesCharacter::ThrowStart()
{
	LanzadoGranada = true;
}

void AMGNGDectectivesCharacter::ThrowOnGoing()
{
	GEngine->AddOnScreenDebugMessage(
			-1,
			1.0f,
			FColor::Red,
			FString::Printf(TEXT("Contando"))
	);
	if (LanzadoGranada && counter >= 0.2f)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			1.0f,
			FColor::Red,
			FString::Printf(TEXT("Contando"))
		);
		Impulso = FMath::Clamp(Impulso, 100.0f, 2000.0f);
		counter = 0;
	}
}

void AMGNGDectectivesCharacter::ThrowRelease()
{
	LanzadoGranada = false;
	/*GranadeBP = Cast<UObject>(StaticLoadObject(UObject::StaticClass(), NULL, TEXT("/Game/BP_Granade.BP_Granade")));
	UBlueprint* GeneratedBP = Cast<UBlueprint>(GranadeBP);
	if(!GranadeBP)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString::Printf(TEXT("CANT FIND OBJECT TO SPAWN")));
		return;
	}
	UClass* SpawnClass = GranadeBP->StaticClass();
	if (SpawnClass == NULL)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString::Printf(TEXT("CLASS == NULL")));
		return;
	}
	
	UWorld* World = GetWorld();
	*/
	AGranade* Granade = GetWorld()->SpawnActor<AGranade>(AGranade::StaticClass(), ArrowDirection->GetComponentLocation(), GetControlRotation());
	Granade->Impulso = Impulso;
	Impulso = 0;
}

void AMGNGDectectivesCharacter::Die(const FInputActionValue& Value)
{
	if(Controller != nullptr)
	{
		isRagdoll = true;
		GetMesh()->SetAllBodiesBelowSimulatePhysics("pelvis", true, true);
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




