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
	bIsGameStartPrompted = false;
	InitialPlayerCount = 0;
	CurrentPlayerTurnIndex = -1;
	bHasGuessedThisTurn = false;
	TurnDuration = 30.0f;
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
		
		bool bWasPlaying = PlayingPlayerStates.Remove(BnCPS) > 0;
		
		FString LogoutMessage = BnCPS->PlayerNameString + TEXT(" has disconnected.");
		BroadcastSystemMessage(LogoutMessage);

		if(bWasPlaying && CurrentGameState == EGameState::Playing)
		{
			if (InitialPlayerCount > 1 && PlayingPlayerStates.Num() == 1)
			{
				// Last player wins in a multiplayer game
				ABnCPlayerState* WinnerPS = PlayingPlayerStates[0];
				if (IsValid(WinnerPS))
				{
					FString WinMessage = WinnerPS->PlayerNameString + TEXT(" has won the game as the last player remaining! The answer was ") + SecretNumberString;
					BroadcastSystemMessage(WinMessage);
				}
				SetGameState(EGameState::GameOver);
				GetWorld()->GetTimerManager().SetTimer(TimerHandle_ResetGame, this, &ABnCGameModeBase::ResetGame, 5.0f, false);
				return;
			}
			
			if (PlayingPlayerStates.Num() > 0)
			{
				// If the player who left was the current turn player, end the turn immediately.
				if(CurrentPlayerTurnIndex >= PlayingPlayerStates.Num())
				{
					CurrentPlayerTurnIndex = 0;
				}
				EndTurn();
			}
			else // No players left
			{
				SetGameState(EGameState::GameOver);
				GetWorld()->GetTimerManager().SetTimer(TimerHandle_ResetGame, this, &ABnCGameModeBase::ResetGame, 5.0f, false);
			}
		}
	}
	
	AllPlayerControllers.Remove(BnCPlayerController);
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
		if (ReadyPlayerStates.Num() >= 1 && ReadyPlayerStates.IsValidIndex(0) && ReadyPlayerStates[0] == BnCPS)
		{
			SetGameState(EGameState::Playing);
		}
	}
	else
	{
		FString ChatMessage = BnCPS->PlayerNameString + TEXT(": ") + InChatMessageString;
		BroadcastChatMessage(ChatMessage);
	}
}

void ABnCGameModeBase::ProcessPlayingMessage(ABnCPlayerController* InChattingPlayerController, const FString& InChatMessageString)
{
	ABnCPlayerState* BnCPS = InChattingPlayerController->GetPlayerState<ABnCPlayerState>();
	if (!IsValid(BnCPS)) return;

	// First, check if the player is a participant or a spectator.
	if (!PlayingPlayerStates.Contains(BnCPS))
	{
		// This player is a spectator. Just broadcast their message.
		FString ChatMessage = BnCPS->PlayerNameString + TEXT("[Spectator]: ") + InChatMessageString;
		BroadcastChatMessage(ChatMessage);
		return;
	}

	// The player is a participant. Now, check if it's their turn.
	if (!PlayingPlayerStates.IsValidIndex(CurrentPlayerTurnIndex) || PlayingPlayerStates[CurrentPlayerTurnIndex] != BnCPS)
	{
		SendTargetedSystemMessage(InChattingPlayerController, TEXT("It's not your turn."));
		return;
	}
	
	if (BnCPS->GetCurrentGuessCount() >= BnCPS->GetMaxGuessCount())
	{
		// This case should ideally not be hit due to draw logic, but as a fallback:
		FString ChatMessage = BnCPS->GetPlayerInfoString() + TEXT(": ") + InChatMessageString;
		BroadcastChatMessage(ChatMessage);
		return;
	}

	if (IsGuessNumberString(InChatMessageString))
	{
		bHasGuessedThisTurn = true;
		
		IncreaseGuessCount(InChattingPlayerController);
		FString ResultString = JudgeResult(SecretNumberString, InChatMessageString);
		
		FString FullMessage = BnCPS->GetPlayerInfoString() + TEXT(": ") + InChatMessageString + TEXT(" -> ") + ResultString;
		BroadcastChatMessage(FullMessage);

		int32 StrikeCount = 0;
		if (ResultString.Equals(TEXT("3S")))
		{
			StrikeCount = 3;
		}
		
		if (JudgeGame(InChattingPlayerController, StrikeCount) == false)
		{
			// If game is not over, end the turn
			EndTurn();
		}
	}
	else
	{
		// Treat as regular chat during the game
		FString ChatMessage = BnCPS->GetPlayerInfoString() + TEXT(": ") + InChatMessageString;
		BroadcastChatMessage(ChatMessage);
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
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_ClearNotification);
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_TurnTimer);
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_TurnTimeUpdater);
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_NextTurn);
	
	bIsGameStartPrompted = false;
	SecretNumberString = GenerateSecretNumber();
	ReadyPlayerStates.Empty();
	PlayingPlayerStates.Empty();
	InitialPlayerCount = 0;
	CurrentPlayerTurnIndex = -1;

	ABnCGameStateBase* BnCGS = GetGameState<ABnCGameStateBase>();
	if(BnCGS)
	{
		BnCGS->CurrentTurnPlayerName = TEXT("");
		BnCGS->TurnTimeRemaining = 0.0f;
	}

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

bool ABnCGameModeBase::JudgeGame(ABnCPlayerController* InChattingPlayerController, int InStrikeCount)
{
	if (InStrikeCount == 3 && InChattingPlayerController != nullptr)
	{
		ABnCPlayerState* WinnerPS = InChattingPlayerController->GetPlayerState<ABnCPlayerState>();
		if (IsValid(WinnerPS))
		{
			FString WinMessage = WinnerPS->PlayerNameString + TEXT(" has won the game!");
			BroadcastSystemMessage(WinMessage);
			SetGameState(EGameState::GameOver);
			GetWorld()->GetTimerManager().SetTimer(TimerHandle_ResetGame, this, &ABnCGameModeBase::ResetGame, 5.0f, false);
			return true;
		}
	}
	
	bool bIsDraw = true;
	for (ABnCPlayerState* PlayingPS : PlayingPlayerStates)
	{
		if (IsValid(PlayingPS) && PlayingPS->GetCurrentGuessCount() < PlayingPS->GetMaxGuessCount())
		{
			bIsDraw = false;
			break;
		}
	}

	if (bIsDraw)
	{
		FString DrawMessage = TEXT("Draw! No one guessed the number. The answer was ") + SecretNumberString;
		BroadcastSystemMessage(DrawMessage);
		SetGameState(EGameState::GameOver);
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_ResetGame, this, &ABnCGameModeBase::ResetGame, 5.0f, false);
		return true;
	}

	return false;
}

void ABnCGameModeBase::SetGameState(EGameState InGameState)
{
	CurrentGameState = InGameState;

	if (CurrentGameState == EGameState::Playing)
	{
		PlayingPlayerStates = ReadyPlayerStates;
		ReadyPlayerStates.Empty();
		bIsGameStartPrompted = false;
		InitialPlayerCount = PlayingPlayerStates.Num();
		
		SecretNumberString = GenerateSecretNumber();

		FString PlayersString;
		for(ABnCPlayerState* PS : PlayingPlayerStates)
		{
			PlayersString += PS->PlayerNameString + TEXT(" ");
		}
		
		BroadcastSystemMessage(FString::Printf(TEXT("Game Start! Players: %s."), *PlayersString));
		
		CurrentPlayerTurnIndex = -1; // StartTurn will advance it to 0
		EndTurn(); // Start the first turn
	}
	else if (InGameState == EGameState::GameOver)
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_TurnTimer);
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_TurnTimeUpdater);
		ABnCGameStateBase* BnCGS = GetGameState<ABnCGameStateBase>();
		if(BnCGS)
		{
			BnCGS->CurrentTurnPlayerName = TEXT("");
			BnCGS->TurnTimeRemaining = 0.0f;
		}
	}
}

void ABnCGameModeBase::BroadcastChatMessage(const FString& InMessage)
{
	ABnCGameStateBase* BnCGS = GetGameState<ABnCGameStateBase>();
	if(IsValid(BnCGS))
	{
		BnCGS->MulticastRPCBroadcastChatMessage(InMessage);
	}
}

void ABnCGameModeBase::BroadcastSystemMessage(const FString& InMessage)
{
	for (ABnCPlayerController* PC : AllPlayerControllers)
	{
		if (IsValid(PC))
		{
			PC->NotificationText = FText::FromString(TEXT("[System] ") + InMessage);
		}
	}

	GetWorld()->GetTimerManager().SetTimer(TimerHandle_ClearNotification, this, &ABnCGameModeBase::ClearAllNotifications, 3.0f, false);
}

void ABnCGameModeBase::SendTargetedSystemMessage(ABnCPlayerController* TargetPC, const FString& InMessage)
{
	if (IsValid(TargetPC))
	{
		TargetPC->NotificationText = FText::FromString(TEXT("[System] ") + InMessage);
	}

	GetWorld()->GetTimerManager().SetTimer(TimerHandle_ClearNotification, this, &ABnCGameModeBase::ClearAllNotifications, 3.0f, false);
}

void ABnCGameModeBase::CheckAllPlayersReady()
{
	if (ReadyPlayerStates.Num() >= 1 && !bIsGameStartPrompted)
	{
		if (ReadyPlayerStates.IsValidIndex(0) && IsValid(ReadyPlayerStates[0]))
		{
			ABnCPlayerController* FirstReadyPC = Cast<ABnCPlayerController>(ReadyPlayerStates[0]->GetPlayerController());
			if(IsValid(FirstReadyPC))
			{
				FString PromptMessage = ReadyPlayerStates[0]->PlayerNameString + TEXT(", you are the first to ready up. Type 'Go' to start the game with the ready players.");
				SendTargetedSystemMessage(FirstReadyPC, PromptMessage);
				bIsGameStartPrompted = true;
			}
		}
	}
}

void ABnCGameModeBase::ClearAllNotifications()
{
	for (ABnCPlayerController* PC : AllPlayerControllers)
	{
		if (IsValid(PC))
		{
			PC->NotificationText = FText::GetEmpty();
		}
	}
}

void ABnCGameModeBase::StartTurn()
{
	if (!PlayingPlayerStates.IsValidIndex(CurrentPlayerTurnIndex))
	{
		return;
	}

	bHasGuessedThisTurn = false;
	ABnCPlayerState* CurrentPlayerPS = PlayingPlayerStates[CurrentPlayerTurnIndex];
	
	ABnCGameStateBase* BnCGS = GetGameState<ABnCGameStateBase>();
	if (BnCGS && CurrentPlayerPS)
	{
		BnCGS->CurrentTurnPlayerName = CurrentPlayerPS->PlayerNameString;
		BnCGS->TurnTimeRemaining = TurnDuration;

		BroadcastSystemMessage(FString::Printf(TEXT("It's %s's turn! (%d seconds)"), *BnCGS->CurrentTurnPlayerName, (int32)TurnDuration));

		GetWorld()->GetTimerManager().SetTimer(TimerHandle_TurnTimer, this, &ABnCGameModeBase::EndTurn, TurnDuration, false);
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_TurnTimeUpdater, this, &ABnCGameModeBase::UpdateTurnTimer, 1.0f, true);
	}
}

void ABnCGameModeBase::EndTurn()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_TurnTimer);
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_TurnTimeUpdater);

	if (CurrentGameState != EGameState::Playing) return;

	if (PlayingPlayerStates.IsValidIndex(CurrentPlayerTurnIndex) && bHasGuessedThisTurn == false)
	{
		ABnCPlayerState* MissedTurnPS = PlayingPlayerStates[CurrentPlayerTurnIndex];
		if (MissedTurnPS)
		{
			ABnCPlayerController* MissedTurnPC = Cast<ABnCPlayerController>(MissedTurnPS->GetPlayerController());
			if (MissedTurnPC)
			{
				IncreaseGuessCount(MissedTurnPC);
				FString MissedTurnMessage = MissedTurnPS->GetPlayerInfoString() + TEXT(" missed their turn!");
				BroadcastChatMessage(MissedTurnMessage);
				
				if (JudgeGame(nullptr, 0))
				{
					return; // Game ended in a draw
				}
			}
		}
	}

	if (PlayingPlayerStates.Num() > 0)
	{
		CurrentPlayerTurnIndex = (CurrentPlayerTurnIndex + 1) % PlayingPlayerStates.Num();
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_NextTurn, this, &ABnCGameModeBase::PrepareNextTurn, 2.0f, false);
	}
	else
	{
		// No one left, end the game
		SetGameState(EGameState::GameOver);
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_ResetGame, this, &ABnCGameModeBase::ResetGame, 5.0f, false);
	}
}

void ABnCGameModeBase::UpdateTurnTimer()
{
	ABnCGameStateBase* BnCGS = GetGameState<ABnCGameStateBase>();
	if (BnCGS)
	{
		BnCGS->TurnTimeRemaining = FMath::Max(0.0f, BnCGS->TurnTimeRemaining - 1.0f);
	}
}

void ABnCGameModeBase::PrepareNextTurn()
{
	StartTurn();
}
