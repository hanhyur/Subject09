// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "BnCGameStateBase.generated.h"

/**
 * 
 */
UCLASS()
class BNC_API ABnCGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPCBroadcastChatMessage(const FString& InMessage);

public:
	UPROPERTY(Replicated, BlueprintReadOnly)
	FString CurrentTurnPlayerName;

	UPROPERTY(Replicated, BlueprintReadOnly)
	float TurnTimeRemaining;
};
