// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BnCGameStateBase.h"

#include "Kismet/GameplayStatics.h"
#include "Player/BnCPlayerController.h"

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
