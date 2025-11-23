// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BnCGameModeBase.h"
#include "Game/BnCGameStateBase.h"
#include "Player/BnCPlayerController.h"
#include "Player/BnCPlayerState.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

ABnCGameModeBase::ABnCGameModeBase()
{
	CurrentGameState = EGameState::Lobby;
}

void ABnCGameModeBase::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	ABnCPlayerController* BnCPlayerController = Cast<ABnCPlayerController>(NewPlayer);
	if (IsValid(BnCPlayerController) == false)
	{
		return;
	}
	
	AllPlayerControllers.Add(BnCPlayerController);

	ABnCPlayerState* BnCPS = BnCPlayerController->GetPlayerState<ABnCPlayerState>();
	if (IsValid(BnCPS) == true)
	{
		FString PlayerName = TEXT("Player") + FString::FromInt(GetNumPlayers());
		BnCPS->PlayerNameString = PlayerName;
		
		FString LoginMessage = PlayerName + TEXT(" has connected.");
		BroadcastSystemMessage(LoginMessage);
	}
}

void ABnCGameModeBase::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	ABnCPlayerController* BnCPlayerController = Cast<ABnCPlayerController>(Exiting);
	if (IsValid(BnCPlayerController) == false)
	{
		return;
	}

	ABnCPlayerState* BnCPS = BnCPlayerController->GetPlayerState<ABnCPlayerState>();
	if(IsValid(BnCPS))
	{
		ReadyPlayerStates.Remove(BnCPS);
		
		FString LogoutMessage = BnCPS->PlayerNameString + TEXT(" has disconnected.");
		BroadcastSystemMessage(LogoutMessage);
	}
	
	AllPlayerControllers.Remove(BnCPlayerController);

	if (CurrentGameState == EGameState::Playing && GetNumPlayers() < 2)
	{
		if (GetNumPlayers() == 1)
		{
			ABnCPlayerController* RemainingPlayer = Cast<ABnCPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
			if(RemainingPlayer)
			{
				ABnCPlayerState* WinnerPS = RemainingPlayer->GetPlayerState<ABnCPlayerState>();
				if (IsValid(WinnerPS))
				{
					FString WinMessage = WinnerPS->PlayerNameString + TEXT(" has won the game. The answer was ") + SecretNumberString;
					BroadcastSystemMessage(WinMessage);
				}
			}
		}
		SetGameState(EGameState::GameOver);
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_ResetGame, this, &ABnCGameModeBase::ResetGame, 5.0f, false);
	}
}

void ABnCGameModeBase::BeginPlay()
{
	Super::BeginPlay();
	SetGameState(EGameState::Lobby);
}

void ABnCGameModeBase::HandleChatMessage(ABnCPlayerController* InChattingPlayerController, const FString& InChatMessageString)
{
	switch (CurrentGameState)
	{
	case EGameState::Lobby:
		ProcessLobbyMessage(InChattingPlayerController, InChatMessageString);
		break;
	case EGameState::Playing:
		ProcessPlayingMessage(InChattingPlayerController, InChatMessageString);
		break;
	case EGameState::GameOver:
		// In GameOver state, only allow system messages or limited chat
		break;
	}
}

void ABnCGameModeBase::ProcessLobbyMessage(ABnCPlayerController* InChattingPlayerController, const FString& InChatMessageString)
{
	ABnCPlayerState* BnCPS = InChattingPlayerController->GetPlayerState<ABnCPlayerState>();
	if (!IsValid(BnCPS)) return;

	if (InChatMessageString.Equals(TEXT("Ready"), ESearchCase::IgnoreCase))
	{
		if (BnCPS->IsReady() == false)
		{
			BnCPS->bIsReady = true;
			if(ReadyPlayerStates.Contains(BnCPS) == false)
			{
				ReadyPlayerStates.Add(BnCPS);
			}
			
			FString ReadyMessage = BnCPS->PlayerNameString + TEXT(" is Ready. (") + FString::FromInt(ReadyPlayerStates.Num()) + TEXT("/") + FString::FromInt(GetNumPlayers()) + TEXT(")");
			BroadcastSystemMessage(ReadyMessage);
			CheckAllPlayersReady();
		}
	}
	else if (InChatMessageString.Equals(TEXT("Go"), ESearchCase::IgnoreCase))
	{
		if (GetNumPlayers() >= 2 && ReadyPlayerStates.Num() == GetNumPlayers() && ReadyPlayerStates.IsValidIndex(0) && ReadyPlayerStates[0] == BnCPS)
		{
			SetGameState(EGameState::Playing);
		}
	}
	else
	{
		FString ChatMessage = BnCPS->GetPlayerInfoString() + TEXT(": ") + InChatMessageString;
		ABnCGameStateBase* BnCGS = GetGameState<ABnCGameStateBase>();
		if(IsValid(BnCGS))
		{
			BnCGS->MulticastRPCBroadcastChatMessage(ChatMessage);
		}
	}
}

void ABnCGameModeBase::ProcessPlayingMessage(ABnCPlayerController* InChattingPlayerController, const FString& InChatMessageString)
{
	ABnCPlayerState* BnCPS = InChattingPlayerController->GetPlayerState<ABnCPlayerState>();
	if (!IsValid(BnCPS) || BnCPS->GetCurrentGuessCount() >= BnCPS->GetMaxGuessCount())
	{
		// If player has no more guesses, treat as regular chat
		FString ChatMessage = BnCPS->GetPlayerInfoString() + TEXT(": ") + InChatMessageString;
		ABnCGameStateBase* BnCGS = GetGameState<ABnCGameStateBase>();
		if(IsValid(BnCGS))
		{
			BnCGS->MulticastRPCBroadcastChatMessage(ChatMessage);
		}
		return;
	}

	if (IsGuessNumberString(InChatMessageString))
	{
		IncreaseGuessCount(InChattingPlayerController);
		FString ResultString = JudgeResult(SecretNumberString, InChatMessageString);
		FString FullMessage = BnCPS->GetPlayerInfoString() + TEXT(": ") + InChatMessageString + TEXT(" -> ") + ResultString;
		
		ABnCGameStateBase* BnCGS = GetGameState<ABnCGameStateBase>();
		if(IsValid(BnCGS))
		{
			BnCGS->MulticastRPCBroadcastChatMessage(FullMessage);
		}

		int32 StrikeCount = 0;
		if (ResultString.Equals(TEXT("3S")))
		{
			StrikeCount = 3;
		}
		JudgeGame(InChattingPlayerController, StrikeCount);
	}
	else
	{
		// Treat as regular chat during the game
		FString ChatMessage = BnCPS->GetPlayerInfoString() + TEXT(": ") + InChatMessageString;
		ABnCGameStateBase* BnCGS = GetGameState<ABnCGameStateBase>();
		if(IsValid(BnCGS))
		{
			BnCGS->MulticastRPCBroadcastChatMessage(ChatMessage);
		}
	}
}

FString ABnCGameModeBase::GenerateSecretNumber()
{
	TArray<int32> Numbers;
	for (int32 i = 1; i <= 9; ++i)
	{
		Numbers.Add(i);
	}

	FString Result;
	for (int32 i = 0; i < 3; ++i)
	{
		int32 Index = FMath::RandRange(0, Numbers.Num() - 1);
		Result.Append(FString::FromInt(Numbers[Index]));
		Numbers.RemoveAt(Index);
	}
	return Result;
}

bool ABnCGameModeBase::IsGuessNumberString(const FString& InNumberString)
{
	if (InNumberString.Len() != 3)
	{
		return false;
	}

	if (!InNumberString.IsNumeric())
	{
		return false;
	}

	TSet<TCHAR> UniqueDigits;
	for (const TCHAR& Char : InNumberString)
	{
		if (Char == '0') return false; // Numbers are 1-9
		UniqueDigits.Add(Char);
	}

	return UniqueDigits.Num() == 3;
}

FString ABnCGameModeBase::JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString)
{
	int32 StrikeCount = 0, BallCount = 0;
	for (int32 i = 0; i < 3; ++i)
	{
		if (InSecretNumberString[i] == InGuessNumberString[i])
		{
			StrikeCount++;
		}
		else if (InSecretNumberString.Contains(FString::Chr(InGuessNumberString[i])))
		{
			BallCount++;
		}
	}

	if (StrikeCount == 3)
	{
		return TEXT("3S");
	}
	
	if (StrikeCount == 0 && BallCount == 0)
	{
		return TEXT("OUT");
	}

	return FString::Printf(TEXT("%dS%dB"), StrikeCount, BallCount);
}

void ABnCGameModeBase::IncreaseGuessCount(ABnCPlayerController* InChattingPlayerController)
{
	ABnCPlayerState* BnCPS = InChattingPlayerController->GetPlayerState<ABnCPlayerState>();
	if (IsValid(BnCPS))
	{
		BnCPS->CurrentGuessCount++;
	}
}

void ABnCGameModeBase::ResetGame()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_ResetGame);
	
	SecretNumberString = GenerateSecretNumber();
	ReadyPlayerStates.Empty();

	for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if(IsValid(PC))
		{
			ABnCPlayerState* BnCPS = PC->GetPlayerState<ABnCPlayerState>();
			if (IsValid(BnCPS))
			{
				BnCPS->CurrentGuessCount = 0;
				BnCPS->bIsReady = false;
			}
		}
	}
	
	BroadcastSystemMessage(TEXT("Game has been reset. Type 'Ready' to play again."));
	SetGameState(EGameState::Lobby);
}

void ABnCGameModeBase::JudgeGame(ABnCPlayerController* InChattingPlayerController, int InStrikeCount)
{
	if (InStrikeCount == 3)
	{
		ABnCPlayerState* WinnerPS = InChattingPlayerController->GetPlayerState<ABnCPlayerState>();
		if (IsValid(WinnerPS))
		{
			FString WinMessage = WinnerPS->PlayerNameString + TEXT(" has won the game!");
			BroadcastSystemMessage(WinMessage);
			SetGameState(EGameState::GameOver);
			GetWorld()->GetTimerManager().SetTimer(TimerHandle_ResetGame, this, &ABnCGameModeBase::ResetGame, 5.0f, false);
		}
	}
	else
	{
		bool bIsDraw = true;
		for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			if(IsValid(PC))
			{
				ABnCPlayerState* BnCPS = PC->GetPlayerState<ABnCPlayerState>();
				if (IsValid(BnCPS) && BnCPS->GetCurrentGuessCount() < BnCPS->GetMaxGuessCount())
				{
					bIsDraw = false;
					break;
				}
			}
		}

		if (bIsDraw)
		{
			FString DrawMessage = TEXT("Draw! No one guessed the number. The answer was ") + SecretNumberString;
			BroadcastSystemMessage(DrawMessage);
			SetGameState(EGameState::GameOver);
			GetWorld()->GetTimerManager().SetTimer(TimerHandle_ResetGame, this, &ABnCGameModeBase::ResetGame, 5.0f, false);
		}
	}
}

void ABnCGameModeBase::SetGameState(EGameState InGameState)
{
	CurrentGameState = InGameState;

	if (CurrentGameState == EGameState::Playing)
	{
		SecretNumberString = GenerateSecretNumber();
		BroadcastSystemMessage(TEXT("Game Start! Guess the 3-digit number."));
	}
}

void ABnCGameModeBase::BroadcastSystemMessage(const FString& InMessage)
{
	ABnCGameStateBase* BnCGS = GetGameState<ABnCGameStateBase>();
	if(IsValid(BnCGS))
	{
		BnCGS->MulticastRPCBroadcastChatMessage(TEXT("[System] ") + InMessage);
	}
}

void ABnCGameModeBase::CheckAllPlayersReady()
{
	if (GetNumPlayers() >= 2 && ReadyPlayerStates.Num() == GetNumPlayers())
	{
		if (ReadyPlayerStates.IsValidIndex(0) && IsValid(ReadyPlayerStates[0]))
		{
			FString PromptMessage = ReadyPlayerStates[0]->PlayerNameString + TEXT(", all players are ready. Type 'Go' to start the game.");
			BroadcastSystemMessage(PromptMessage);
		}
	}
}
