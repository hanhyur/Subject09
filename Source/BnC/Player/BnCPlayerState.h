// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BnCPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class BNC_API ABnCPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ABnCPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	FString GetPlayerInfoString();

	UFUNCTION(BlueprintCallable)
	int32 GetCurrentGuessCount() const { return CurrentGuessCount; }

	UFUNCTION(BlueprintCallable)
	int32 GetMaxGuessCount() const { return MaxGuessCount; }

	UFUNCTION(BlueprintCallable)
	bool IsReady() const { return bIsReady; }
	
public:
	UPROPERTY(Replicated)
	FString PlayerNameString;

	UPROPERTY(Replicated)
	int32 CurrentGuessCount;

	UPROPERTY(Replicated)
	int32 MaxGuessCount;

	UPROPERTY(Replicated)
	bool bIsReady;
	
};
