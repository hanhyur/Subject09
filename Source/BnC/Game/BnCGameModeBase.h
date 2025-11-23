// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BnCGameModeBase.generated.h"

class ABnCPlayerController;
class ABnCPlayerState;

UENUM(BlueprintType)
enum class EGameState : uint8
{
	Lobby,
	Playing,
	GameOver
};

/**
 * 
 */
UCLASS()
class BNC_API ABnCGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABnCGameModeBase();
	
	virtual void OnPostLogin(AController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void BeginPlay() override;

	void HandleChatMessage(ABnCPlayerController* InChattingPlayerController, const FString& InChatMessageString);

protected:
	void ProcessLobbyMessage(ABnCPlayerController* InChattingPlayerController, const FString& InChatMessageString);
	void ProcessPlayingMessage(ABnCPlayerController* InChattingPlayerController, const FString& InChatMessageString);
	
	FString GenerateSecretNumber();
	bool IsGuessNumberString(const FString& InNumberString);
	FString JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString);

	void IncreaseGuessCount(ABnCPlayerController* InChattingPlayerController);
	void ResetGame();
	bool JudgeGame(ABnCPlayerController* InChattingPlayerController, int InStrikeCount);

	void SetGameState(EGameState InGameState);
	void BroadcastChatMessage(const FString& InMessage);
	void BroadcastSystemMessage(const FString& InMessage);
	void SendTargetedSystemMessage(ABnCPlayerController* TargetPC, const FString& InMessage);
	void CheckAllPlayersReady();

	void ClearAllNotifications();

	void StartTurn();
	void EndTurn();
	void UpdateTurnTimer();
	void PrepareNextTurn();
	
protected:
	FString SecretNumberString;

	TArray<TObjectPtr<ABnCPlayerController>> AllPlayerControllers;

	EGameState CurrentGameState;

	UPROPERTY()
	TArray<TObjectPtr<ABnCPlayerState>> ReadyPlayerStates;

	UPROPERTY()
	TArray<TObjectPtr<ABnCPlayerState>> PlayingPlayerStates;

	bool bIsGameStartPrompted;

	int32 InitialPlayerCount;
	int32 CurrentPlayerTurnIndex;
	bool bHasGuessedThisTurn;
	float TurnDuration;

	FTimerHandle TimerHandle_ResetGame;
	FTimerHandle TimerHandle_ClearNotification;
	FTimerHandle TimerHandle_TurnTimer;
	FTimerHandle TimerHandle_TurnTimeUpdater;
	FTimerHandle TimerHandle_NextTurn;
};
