#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Components/DecalComponent.h"
#include "ItemActor.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MGNGDectectivesCharacter.generated.h"


UCLASS(config=Game)
class AMGNGDectectivesCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;
	
	//Die Action
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* DieAction;

	//Throw Action
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* ThrowAction;

	//Throw Released
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* ThrowReleased;
	
	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Direction, meta = (AllowPrivateAccess = "true"))
	class UArrowComponent* ArrowDirection;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Default, meta = (AllowPrivateAccess = "true"))
	class UDecalComponent* DecalComponent;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* PickAction;

	
      /*                             
    
    */
    
   
    
	
public:
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	class UAnimMontage* PickAnimation;
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
    class UAnimMontage* ShootAnimation;
	UPROPERTY(EditAnywhere,Category = Item)
        AItemActor* itemClass;
	AMGNGDectectivesCharacter();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	TSubclassOf<AActor>Granada;
	IOnlineSessionPtr OnlineSessionInterface;
	

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Keys)
	int Piece = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Weapon)
	bool canSoot = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Weapon)
	float granadeOpacity = 1.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Death)
	bool tieso = false;

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	void Die(const FInputActionValue& Value);
	
	void ThrowStart();
	
	void ThrowRelease();

	void PrintOnDebug(FString TextToDisplay);

	void PickUp();


protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay();

	virtual void Tick(float DeltaSeconds) override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable)
	void CreateGameSession();

	UFUNCTION(BlueprintCallable)
	void JoinGameSession();

	//Callbacks
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccess);
	void OnFindSessionComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	
	bool isRagdoll;
	
	bool LanzadoGranada;
	float Impulso;
	float counter;

	bool StartCount;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category=Weapon)
    bool canPick = false;
	FHitResult HitResults;
	TArray<FVector> OutPathPositions;
	FVector OutLastTraceDestinations;

	TArray<AActor*> ActorsToIgnore;
	FRotator MyRotator;
	FVector ForwardVector;
	
	FVector StartLocation;
	FVector LaunchVelocity;

	bool bTraceComplex;
	TEnumAsByte<ECollisionChannel> CollisionChannel;

	FCollisionQueryParams Params;
private:
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;

	TSharedPtr<FOnlineSessionSearch> SessionSearch;
};

