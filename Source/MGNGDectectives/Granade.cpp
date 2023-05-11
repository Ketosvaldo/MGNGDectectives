// Fill out your copyright notice in the Description page of Project Settings.


#include "Granade.h"

#include "MGNGDectectivesCharacter.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AGranade::AGranade()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	GranadeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GranadeMesh"));
	GranadeMesh->SetupAttachment(RootComponent);

	FString MeshPath = TEXT("/Game/StarterContent/Shapes/Shape_Cylinder.Shape_Cylinder");
	UStaticMesh* GranadeMeshAsset = LoadObject<UStaticMesh>(nullptr, *MeshPath);
	GranadeMesh->SetStaticMesh(GranadeMeshAsset);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = Impulso;

	FString SoundPath = TEXT("/Game/StarterContent/Audio/Explosion01.Explosion01");
	ExplosionSound = LoadObject<USoundBase>(nullptr, *SoundPath);

	RadialForce = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForce"));
	RadialForce->SetupAttachment(RootComponent);
	RadialForce->SetWorldLocation(GranadeMesh->GetComponentLocation());
	RadialForce->Radius = 500.0f;
	RadialForce->ImpulseStrength = 200000.0f;

	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereCollision->SetupAttachment(GranadeMesh);
	SphereCollision->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OverlapBegin);
	counter = 0;
}

// Called when the game starts or when spawned
void AGranade::BeginPlay()
{
	Super::BeginPlay();
}

void AGranade::Tick(float DeltaSeconds)
{
	RadialForce->SetWorldLocation(GranadeMesh->GetComponentLocation());
	SphereCollision->SetWorldLocation(GranadeMesh->GetComponentLocation());
	/*counter += DeltaSeconds;
	if(counter >= 3.0f)
	{
		UWorld* World = GetWorld();
		UGameplayStatics::ApplyRadialDamage(World, RadialForce->ImpulseStrength, GetActorLocation(), RadialForce->Radius, nullptr, IgnoreActors);
		RadialForce->FireImpulse();
		UGameplayStatics::SpawnSound2D(World, ExplosionSound, 1.0f,1.0f,0.0f,nullptr,false,true);
		UGameplayStatics::SpawnEmitterAtLocation(World, ExplosionParticles, GetActorLocation());
		Destroy();
	}*/
}

void AGranade::OverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	
	if ((Character != nullptr))
	{
		UWorld* World = GetWorld();
		UGameplayStatics::ApplyRadialDamage(World, RadialForce->ImpulseStrength, GetActorLocation(), RadialForce->Radius, nullptr, IgnoreActors);
		RadialForce->FireImpulse();
		UGameplayStatics::SpawnSound2D(World, ExplosionSound, 1.0f,1.0f,0.0f,nullptr,false,true);
		UGameplayStatics::SpawnEmitterAtLocation(World, ExplosionParticles, GetActorLocation());
		Destroy();
	}
}


