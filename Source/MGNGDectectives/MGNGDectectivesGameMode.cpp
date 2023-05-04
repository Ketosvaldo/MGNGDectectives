// Copyright Epic Games, Inc. All Rights Reserved.

#include "MGNGDectectivesGameMode.h"
#include "MGNGDectectivesCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMGNGDectectivesGameMode::AMGNGDectectivesGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
