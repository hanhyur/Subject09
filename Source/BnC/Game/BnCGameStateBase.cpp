// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BnCGameStateBase.h"

#include "Kismet/GameplayStatics.h"
#include "Player/BnCPlayerController.h"
#include "Net/UnrealNetwork.h"

void ABnCGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABnCGameStateBase, CurrentTurnPlayerName);
	DOREPLIFETIME(ABnCGameStateBase, TurnTimeRemaining);
}

void ABnCGameStateBase::MulticastRPCBroadcastChatMessage_Implementation(const FString& InMessage)
{
	if (HasAuthority() == false)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);

		if (IsValid(PC) == true)
		{
			ABnCPlayerController* BnCPC = Cast<ABnCPlayerController>(PC);

			if (IsValid(BnCPC) == true)
			{
				BnCPC->PrintChatMessageString(InMessage);
			}
		}
	}
}
