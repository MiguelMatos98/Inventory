// Copyright Epic Games, Inc. All Rights Reserved.

#include "UEInventoryGameMode.h"
#include "UEInventoryCharacter.h"
#include "UObject/ConstructorHelpers.h"

AUEInventoryGameMode::AUEInventoryGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
