// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BnCPlayerController.generated.h"

class UBnCChatInput;

/**
 * 
 */
UCLASS()
class BNC_API ABnCPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABnCPlayerController();
	
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	void SetChatMessageString(const FString& InChatMessageString);

	void PrintChatMessageString(const FString& InChatMessageString);

	UFUNCTION(Client, Reliable)
	void ClientRPCPrintChatMessageString(const FString& InChatMessageString);

	UFUNCTION(Server, Reliable)
	void ServerRPCPrintChatMessageString(const FString& InChatMessageString);

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	void FocusChatInput();
	
protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UBnCChatInput> ChatInputWidgetClass;

	UPROPERTY()
	TObjectPtr<UBnCChatInput> ChatInputWidgetInstance;

	FString ChatMessageString;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> NotificationTextWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> NotificationTextWidgetInstance;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> TurnInfoWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> TurnInfoWidgetInstance;

public:
	UPROPERTY(Replicated, BlueprintReadOnly)
	FText NotificationText;
	
};
