// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/BnCPlayerController.h"

#include "BnC.h"
#include "UI/BnCChatInput.h"
#include "Kismet/KismetSystemLibrary.h"
#include "EngineUtils.h"
#include "Game/BnCGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "BnCPlayerState.h"
#include "Components/EditableTextBox.h"
#include "Components/InputComponent.h"
#include "Net/UnrealNetwork.h"

ABnCPlayerController::ABnCPlayerController()
{
	bReplicates = true;
}

void ABnCPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() == false)
	{
		return;
	}

	if (IsValid(ChatInputWidgetClass) == true)
	{
		ChatInputWidgetInstance = CreateWidget<UBnCChatInput>(this, ChatInputWidgetClass);

		if (IsValid(ChatInputWidgetInstance) == true)
		{
			ChatInputWidgetInstance->AddToViewport();
		}
	}

	if (IsValid(NotificationTextWidgetClass) == true)
	{
		NotificationTextWidgetInstance = CreateWidget<UUserWidget>(this, NotificationTextWidgetClass);

		if (IsValid(NotificationTextWidgetInstance) == true)
		{
			NotificationTextWidgetInstance->AddToViewport();
		}
	}

	if (IsValid(TurnInfoWidgetClass) == true)
	{
		TurnInfoWidgetInstance = CreateWidget<UUserWidget>(this, TurnInfoWidgetClass);

		if (IsValid(TurnInfoWidgetInstance) == true)
		{
			TurnInfoWidgetInstance->AddToViewport();
		}
	}

	// Set to GameAndUI mode and focus chat input by default.
	FocusChatInput();
}

void ABnCPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent)
	{
		InputComponent->BindAction("Chat", IE_Pressed, this, &ABnCPlayerController::FocusChatInput);
	}
}

void ABnCPlayerController::FocusChatInput()
{
	if (IsValid(ChatInputWidgetInstance) && IsValid(ChatInputWidgetInstance->EditableTextBox_ChatInput))
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(ChatInputWidgetInstance->EditableTextBox_ChatInput->TakeWidget());
		SetInputMode(InputMode);
		bShowMouseCursor = true;
	}
}

void ABnCPlayerController::SetChatMessageString(const FString& InChatMessageString)
{
	if (IsLocalController() == true)
	{
		ServerRPCPrintChatMessageString(InChatMessageString);
	}
}

void ABnCPlayerController::PrintChatMessageString(const FString& InChatMessageString)
{
	// UKismetSystemLibrary::PrintString(
	// 	this,
	// 	ChatMessageString,
	// 	true,
	// 	true,
	// 	FLinearColor::Red,
	// 	5.0f);

	// FString NetModeString = BnCFunctionLibrary::GetNetModeString(this);
	// FString CombinedMessageString = FString::Printf(TEXT("%s: %s"), *NetModeString, *InChatMessageString);
	// BnCFunctionLibrary::MyPrintString(this, CombinedMessageString, 10.f);

	BnCFunctionLibrary::MyPrintString(this, InChatMessageString, 10.f);
}

void ABnCPlayerController::ClientRPCPrintChatMessageString_Implementation(const FString& InChatMessageString)
{
	PrintChatMessageString(InChatMessageString);
}

void ABnCPlayerController::ServerRPCPrintChatMessageString_Implementation(const FString& InChatMessageString)
{
	// for (TActorIterator<ABnCPlayerController> It(GetWorld()); It; ++It)
	// {
	// 	ABnCPlayerController* BnCPlayerController = *It;
	// 	if (IsValid(BnCPlayerController) == true)
	// 	{
	// 		BnCPlayerController->ClientRPCPrintChatMessageString(InChatMessageString);
	// 	}
	// }

	AGameModeBase* GM = UGameplayStatics::GetGameMode(this);

	if (IsValid(GM) == true)
	{
		ABnCGameModeBase* BnCGM = Cast<ABnCGameModeBase>(GM);

		if (IsValid(BnCGM) == true)
		{
			BnCGM->HandleChatMessage(this, InChatMessageString);
		}
	}
}

void ABnCPlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, NotificationText);
}
