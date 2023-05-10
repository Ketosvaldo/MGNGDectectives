// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemActor.h"

#include "MGNGDectectivesCharacter.h"
#include "Components/BoxComponent.h"

// Sets default values
AItemActor::AItemActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;


	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OverlapBegin);
	CollisionBox->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OverlapEnd);

}

// Called when the game starts or when spawned
void AItemActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void AItemActor::OverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

	AMGNGDectectivesCharacter* Character = Cast<AMGNGDectectivesCharacter>(OtherActor);

	if(Character != nullptr)
	{
		Character->canPick = true;
		Character->itemClass = this;
		
	}

}

void AItemActor::OverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{

	AMGNGDectectivesCharacter* Character = Cast<AMGNGDectectivesCharacter>(OtherActor);
	

	if(Character != nullptr)
	{
		Character->canPick = false;
		Character->itemClass = nullptr;
	}
	

}

// Called every frame
void AItemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

