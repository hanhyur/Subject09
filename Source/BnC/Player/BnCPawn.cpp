// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/BnCPawn.h"

#include "BnC.h"

// Called when the game starts or when spawned
void ABnCPawn::BeginPlay()
{
	Super::BeginPlay();

	FString NetRoleString = BnCFunctionLibrary::GetRoleString(this);
	FString CombinedString = FString::Printf(
		TEXT("BnCPawn::BeginPlay() %s [%s]"),
		*BnCFunctionLibrary::GetNetModeString(this),
		*NetRoleString);
	BnCFunctionLibrary::MyPrintString(this, CombinedString, 10.f);
}

void ABnCPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	FString NetRoleString = BnCFunctionLibrary::GetRoleString(this);
	FString CombinedString = FString::Printf(
		TEXT("BnCPawn::PossessedBy() %s [%s]"),
		*BnCFunctionLibrary::GetNetModeString(this),
		*NetRoleString);
	BnCFunctionLibrary::MyPrintString(this, CombinedString, 10.f);
}
