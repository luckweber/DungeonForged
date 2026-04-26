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
	FInputModeUIOnly Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	ForPC->SetInputMode(Mode);
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
			// Foco do utilizador para que as teclas Skip funcionem em qualquer momento.
			Mode.SetWidgetToFocus(SplashWgt->TakeWidget());
			ForPC->SetInputMode(Mode);
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
		// Foco no menu principal para navegação por teclado/gamepad.
		FInputModeUIOnly Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetWidgetToFocus(Main->TakeWidget());
		PC->SetInputMode(Mode);
	}
	// A seleção de perfil abre a partir de "Nova Aventura" / "Continuar" (UDFMainMenuUserWidget
	// e ADFMainMenuHUD::ShowSaveSlotLayer), nunca automaticamente no primeiro frame — o menu
	// fica visível abaixo (Z 5) e o overlay de slots (Z 20) só em fluxo explícito do jogador.
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
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetWidgetToFocus(SaveSlotLayer->TakeWidget());
		PC->SetInputMode(InputMode);
	}
}

void ADFMainMenuHUD::HideSaveSlotLayer()
{
	if (SaveSlotLayer)
	{
		SaveSlotLayer->RemoveFromParent();
	}
	SaveSlotLayer = nullptr;
	RestoreMainMenuFocus();
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

void ADFMainMenuHUD::RestoreMainMenuFocus()
{
	APlayerController* const PC = GetOwningPlayerController();
	if (!PC || !Main || !Main->IsInViewport())
	{
		return;
	}
	FInputModeUIOnly Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetWidgetToFocus(Main->TakeWidget());
	PC->SetInputMode(Mode);
	Main->RefreshForCurrentSaveState();
}
