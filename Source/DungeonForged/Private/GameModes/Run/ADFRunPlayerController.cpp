// Source/DungeonForged/Private/GameModes/Run/ADFRunPlayerController.cpp
#include "GameModes/Run/ADFRunPlayerController.h"
#include "GameModes/Run/UDFVictoryScreenWidget.h"
#include "GameModes/Run/UDFDefeatScreenWidget.h"
#include "World/DFWorldTypes.h"
#include "World/UDFWorldTransitionSubsystem.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputSubsystems.h"

ADFRunPlayerController::ADFRunPlayerController() = default;

void ADFRunPlayerController::SetupInputModeGameplay()
{
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	SetShowMouseCursor(false);
	SetInputMode(FInputModeGameOnly());

	if (UEnhancedInputLocalPlayerSubsystem* const Sub =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (UInputMappingContext* const IMC = DefaultGameplayIMC
				? DefaultGameplayIMC.Get()
				: (GetPawn<ADFPlayerCharacter>() ? GetPawn<ADFPlayerCharacter>()->IMC_Default.Get() : nullptr))
		{
			Sub->AddMappingContext(IMC, IMC_Priority);
		}
	}
}

void ADFRunPlayerController::SetupInputModeUI()
{
	UGameplayStatics::SetGamePaused(GetWorld(), true);
	SetShowMouseCursor(true);
	FInputModeUIOnly M;
	if (UUserWidget* const W = CharacterScreenInstance ? CharacterScreenInstance : PauseMenuInstance)
	{
		M.SetWidgetToFocus(W->TakeWidget());
	}
	SetInputMode(M);
}

void ADFRunPlayerController::ToggleInventory()
{
	if (CharacterScreenInstance)
	{
		CloseCharacterScreen();
		SetupInputModeGameplay();
		return;
	}
	if (!CharacterScreenClass)
	{
		return;
	}
	CharacterScreenInstance = CreateWidget<UUserWidget>(this, CharacterScreenClass);
	if (CharacterScreenInstance)
	{
		CharacterScreenInstance->AddToViewport(15);
		SetupInputModeUI();
	}
}

void ADFRunPlayerController::OnPause()
{
	if (PauseMenuInstance)
	{
		ClosePauseMenu();
		SetupInputModeGameplay();
		return;
	}
	if (!PauseMenuClass)
	{
		return;
	}
	PauseMenuInstance = CreateWidget<UUserWidget>(this, PauseMenuClass);
	if (PauseMenuInstance)
	{
		PauseMenuInstance->AddToViewport(20);
		UGameplayStatics::SetGamePaused(GetWorld(), true);
		SetShowMouseCursor(true);
		FInputModeUIOnly M;
		M.SetWidgetToFocus(PauseMenuInstance->TakeWidget());
		SetInputMode(M);
	}
}

void ADFRunPlayerController::CloseCharacterScreen()
{
	if (CharacterScreenInstance)
	{
		CharacterScreenInstance->RemoveFromParent();
		CharacterScreenInstance = nullptr;
	}
}

void ADFRunPlayerController::ClosePauseMenu()
{
	if (PauseMenuInstance)
	{
		PauseMenuInstance->RemoveFromParent();
		PauseMenuInstance = nullptr;
	}
	UGameplayStatics::SetGamePaused(GetWorld(), false);
}

void ADFRunPlayerController::Client_OpenVictoryScreen_Implementation(const FDFRunSummary Summary)
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.3f);
	if (VictoryScreenWidgetClass)
	{
		if (UDFVictoryScreenWidget* const W = CreateWidget<UDFVictoryScreenWidget>(this, VictoryScreenWidgetClass))
		{
			W->SetSummary(Summary);
			W->AddToViewport(20);
		}
	}
}

void ADFRunPlayerController::Client_OpenDefeatScreen_Implementation(
	const FDFRunSummary Summary, const FString& DefeatCause)
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
	if (DefeatScreenWidgetClass)
	{
		if (UDFDefeatScreenWidget* const W = CreateWidget<UDFDefeatScreenWidget>(this, DefeatScreenWidgetClass))
		{
			const FText CauseText = FText::FromString(DefeatCause);
			W->SetDefeatData(Summary, CauseText, FText::GetEmpty());
			W->AddToViewport(20);
		}
	}
}

void ADFRunPlayerController::PresentBetweenFloorFlow_Implementation() {}

void ADFRunPlayerController::Client_PresentBetweenFloorUI_Implementation()
{
	PresentBetweenFloorFlow();
}

void ADFRunPlayerController::Server_FinishBetweenFloorUI_Implementation() {}

bool ADFRunPlayerController::Server_FinishBetweenFloorUI_Validate()
{
	return true;
}

void ADFRunPlayerController::RequestReturnToNexus(const ERunNexusTravelReason Reason)
{
	if (IsLocalController())
	{
		Server_RequestReturnToNexus(Reason);
	}
}

void ADFRunPlayerController::Server_RequestReturnToNexus_Implementation(ERunNexusTravelReason const Reason)
{
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFWorldTransitionSubsystem* const T = GI->GetSubsystem<UDFWorldTransitionSubsystem>())
		{
			T->TravelToNexus(DFWorldTransition::NexusReasonToTravel(Reason));
		}
	}
}

bool ADFRunPlayerController::Server_RequestReturnToNexus_Validate(ERunNexusTravelReason const /*Reason*/)
{
	return true;
}

void ADFRunPlayerController::RequestPlayAgain() {}
