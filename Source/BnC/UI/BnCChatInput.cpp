// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/BnCChatInput.h"

#include "Components/EditableTextBox.h"
#include "Player/BnCPlayerController.h"

void UBnCChatInput::NativeConstruct()
{
	Super::NativeConstruct();

	if (EditableTextBox_ChatInput
		->OnTextCommitted.IsAlreadyBound(this, &ThisClass::OnChatInputTextCommitted) == false)
	{
		EditableTextBox_ChatInput->OnTextCommitted.AddDynamic(this, &ThisClass::OnChatInputTextCommitted);
	}
}

void UBnCChatInput::NativeDestruct()
{
	Super::NativeDestruct();

	if (EditableTextBox_ChatInput
		->OnTextCommitted.IsAlreadyBound(this, &ThisClass::OnChatInputTextCommitted) == false)
	{
		EditableTextBox_ChatInput->OnTextCommitted.RemoveDynamic(this, &ThisClass::OnChatInputTextCommitted);
	}
}

void UBnCChatInput::OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		APlayerController* OwningPlayerController = GetOwningPlayer();

		if (IsValid(OwningPlayerController) == true)
		{
			ABnCPlayerController* OwningBnCPlayerController = Cast<ABnCPlayerController>(OwningPlayerController);

			if (IsValid(OwningBnCPlayerController) == true)
			{
				if (Text.IsEmpty() == false)
				{
					OwningBnCPlayerController->SetChatMessageString(Text.ToString());
				}
				
				EditableTextBox_ChatInput->SetText(FText());

				// Re-focus the chat input to allow for continuous chatting.
				OwningBnCPlayerController->FocusChatInput();
			}
		}
	}
}

