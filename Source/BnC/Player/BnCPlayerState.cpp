// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/BnCPlayerState.h"

#include "Net/UnrealNetwork.h"

ABnCPlayerState::ABnCPlayerState()
	: PlayerNameString(TEXT("None")),
	  CurrentGuessCount(0),
	  MaxGuessCount(3),
	  bIsReady(false)
{
	bReplicates = true;
}

void ABnCPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, PlayerNameString);
	DOREPLIFETIME(ThisClass, CurrentGuessCount);
	DOREPLIFETIME(ThisClass, MaxGuessCount);
	DOREPLIFETIME(ThisClass, bIsReady);
}

FString ABnCPlayerState::GetPlayerInfoString()
{
	FString PlayerInfoString = PlayerNameString + TEXT("(") + FString::FromInt(CurrentGuessCount) + TEXT("/") +
		FString::FromInt(MaxGuessCount) + TEXT(")");

	return PlayerInfoString;
}
