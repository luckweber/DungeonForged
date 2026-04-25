// Source/DungeonForged/Private/GameModes/MainMenu/ADFMainMenuHUD.cpp
#include "GameModes/MainMenu/ADFMainMenuHUD.h"
#include "GameModes/MainMenu/UDFSplashScreenUserWidget.h"
#include "GameModes/MainMenu/UDFMainMenuUserWidget.h"
#include "GameModes/MainMenu/UDFConfirmDialogUserWidget.h"
#include "GameModes/MainMenu/UDFCreditsUserWidget.h"
#include "GameModes/MainMenu/UDFSaveSlotSelectionUserWidget.h"
#include "Run/UDFSaveSlotManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"

ADFMainMenuHUD::ADFMainMenuHUD() = default;

void ADFMainMenuHUD::BeginPlay()
{
	Super::BeginPlay();
}

void ADFMainMenuHUD::OnLocalPlayerMenuReady(APlayerController* const ForPC)
{
	if (!ForPC || !ForPC->IsLocalController())
	{
		return;
	}
	if (UWorld* const W = GetWorld())
	{
		if (W->GetNetMode() == NM_DedicatedServer)
		{
			return;
		}
	}
	ForPC->SetInputMode(FInputModeUIOnly());
	ForPC->bShowMouseCursor = true;
	if (SplashWidgetClass)
	{
		UDFSplashScreenUserWidget* const SplashWgt =
			CreateWidget<UDFSplashScreenUserWidget>(ForPC, SplashWidgetClass);
		Splash = SplashWgt;
		if (SplashWgt)
		{
			SplashWgt->SetOwnerHUD(this);
			SplashWgt->AddToViewport(0);
			SplashWgt->StartSplashFlow();
		}
	}
	else
	{
		ShowMainMenu();
	}
}

void ADFMainMenuHUD::ShowMainMenu()
{
	if (Splash)
	{
		Splash->RemoveFromParent();
		Splash = nullptr;
	}
	APlayerController* const PC = GetOwningPlayerController();
	if (!PC || !MainMenuWidgetClass)
	{
		return;
	}
	if (!Main)
	{
		Main = CreateWidget<UDFMainMenuUserWidget>(PC, MainMenuWidgetClass);
	}
	if (Main)
	{
		Main->AddToViewport(5);
		Main->RefreshForCurrentSaveState();
	}
	UGameInstance* const GI = GetGameInstance();
	UDFSaveSlotManagerSubsystem* const Slots = GI ? GI->GetSubsystem<UDFSaveSlotManagerSubsystem>() : nullptr;
	if (Slots)
	{
		// Instalação: nenhum perfil; painel de slot antes do menu, se desejado.
		if (!Slots->HasAnyProfileOrLegacySave())
		{
			ShowSaveSlotLayer(EDFSlotScreenMode::SelectToPlay);
		}
	}
}

void ADFMainMenuHUD::ShowSaveSlotLayer(EDFSlotScreenMode const Mode)
{
	if (!SaveSlotWidgetClass)
	{
		return;
	}
	APlayerController* const PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}
	if (!SaveSlotLayer)
	{
		SaveSlotLayer = CreateWidget<UDFSaveSlotSelectionUserWidget>(PC, SaveSlotWidgetClass);
	}
	if (SaveSlotLayer)
	{
		SaveSlotLayer->SetScreenMode(Mode);
		SaveSlotLayer->RefreshFromSubsystem();
		SaveSlotLayer->AddToViewport(20);
	}
}

void ADFMainMenuHUD::HideSaveSlotLayer()
{
	if (SaveSlotLayer)
	{
		SaveSlotLayer->RemoveFromParent();
	}
	SaveSlotLayer = nullptr;
}

void ADFMainMenuHUD::ShowCredits()
{
	if (!Credits && CreditsWidgetClass)
	{
		if (APlayerController* const PC = GetOwningPlayerController())
		{
			Credits = CreateWidget<UDFCreditsUserWidget>(PC, CreditsWidgetClass);
		}
	}
	if (Credits)
	{
		Credits->AddToViewport(15);
	}
}

void ADFMainMenuHUD::ShowConfirmDialog(UDFConfirmDialogUserWidget* const Inst)
{
	if (Inst)
	{
		Inst->AddToViewport(100);
	}
}
