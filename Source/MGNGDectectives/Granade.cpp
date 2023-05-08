// Fill out your copyright notice in the Description page of Project Settings.


#include "Granade.h"
#include <chrono>
#include <thread>
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
}

// Called when the game starts or when spawned
void AGranade::BeginPlay()
{
	Super::BeginPlay();
	constexpr  std::chrono::duration<float> Duration(3.0f);
	std::this_thread::sleep_for(Duration);
	UGameplayStatics::SpawnSound2D(GetWorld(), ExplosionSound, 1.0f,1.0f,0.0f,nullptr,false,true);
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionParticles, GetActorLocation());
	Destroy();
}

