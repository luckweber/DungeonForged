// Source/DungeonForged/Private/GameModes/MainMenu/ADFMainMenuHUD.cpp
#include "GameModes/MainMenu/ADFMainMenuHUD.h"
#include "DungeonForgedModule.h"
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
			SplashWgt->AddToViewport(DFMainMenuUI::ViewportZ_Splash);
			// Foco do utilizador para que as teclas Skip funcionem em qualquer momento.
			DFPrepareWidgetForUIModeFocus(SplashWgt);
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
		DF_LOG(Warning, "[DF|MainMenu|HUD] ShowMainMenu: abort (PC=%s MainMenuWidgetClass=%s)",
			PC ? TEXT("ok") : TEXT("null"),
			MainMenuWidgetClass ? *MainMenuWidgetClass->GetName() : TEXT("null"));
		return;
	}
	if (!Main)
	{
		Main = CreateWidget<UDFMainMenuUserWidget>(PC, MainMenuWidgetClass);
		if (Main)
		{
			DF_LOG(Log, "[DF|MainMenu|HUD] ShowMainMenu: criou Main Z=%d instancia=%s",
				DFMainMenuUI::ViewportZ_Main,
				*Main->GetClass()->GetName());
		}
		else
		{
			DF_LOG(Error, "[DF|MainMenu|HUD] ShowMainMenu: CreateWidget falhou (classe BP=%s)",
				*MainMenuWidgetClass->GetName());
		}
	}
	if (Main)
	{
		Main->AddToViewport(DFMainMenuUI::ViewportZ_Main);
		Main->RefreshForCurrentSaveState();
		// Foco no menu principal para navegação por teclado/gamepad.
		FInputModeUIOnly Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		DFPrepareWidgetForUIModeFocus(Main);
		Mode.SetWidgetToFocus(Main->TakeWidget());
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = true;
	}
	// A seleção de perfil abre a partir de "Nova Aventura" / "Continuar" (UDFMainMenuUserWidget
	// e ADFMainMenuHUD::ShowSaveSlotLayer), nunca automaticamente no primeiro frame — o menu
	// fica visível abaixo (Z 5) e o overlay de slots (Z 20) só em fluxo explícito do jogador.
}

void ADFMainMenuHUD::ShowSaveSlotLayer(EDFSlotScreenMode const Mode)
{
	if (!SaveSlotWidgetClass)
	{
		DF_LOG(Warning, "[DF|MainMenu|HUD] ShowSaveSlotLayer: SaveSlotWidgetClass nulo (configure no BP_DFMainMenuHUD)");
		return;
	}
	APlayerController* const PC = GetOwningPlayerController();
	if (!PC)
	{
		DF_LOG(Warning, "[DF|MainMenu|HUD] ShowSaveSlotLayer: PlayerController nulo");
		return;
	}
	const bool bFirstConstruction = !SaveSlotLayer;
	if (!SaveSlotLayer)
	{
		SaveSlotLayer = CreateWidget<UDFSaveSlotSelectionUserWidget>(PC, SaveSlotWidgetClass);
		if (SaveSlotLayer)
		{
			DF_LOG(Log, "[DF|MainMenu|HUD] ShowSaveSlotLayer: primeira criacao BP=%s instancia=%s",
				*SaveSlotWidgetClass->GetName(),
				*SaveSlotLayer->GetClass()->GetName());
		}
		else
		{
			DF_LOG(Error, "[DF|MainMenu|HUD] ShowSaveSlotLayer: CreateWidget falhou (BP=%s)",
				*SaveSlotWidgetClass->GetName());
			return;
		}
	}
	if (SaveSlotLayer)
	{
		// AddToViewport antes de refresh: o NativeConstruct do widget já chama
		// RefreshFromSubsystem; chamar antes faria SlotRow->ClearChildren rodar
		// duas vezes e podia deixar bindings de OnClicked em estado inconsistente.
		SaveSlotLayer->AddToViewport(DFMainMenuUI::ViewportZ_SaveSlot);

		SaveSlotLayer->SetScreenMode(Mode);

		if (!bFirstConstruction)
		{
			// Reuso do widget cacheado: NativeConstruct nao roda de novo, entao
			// repopulamos manualmente respeitando o novo Mode.
			SaveSlotLayer->RefreshFromSubsystem();
		}
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		DFPrepareWidgetForUIModeFocus(SaveSlotLayer);
		InputMode.SetWidgetToFocus(SaveSlotLayer->TakeWidget());
		PC->SetInputMode(InputMode);
		// Garante que o cursor esteja visivel mesmo se algum fluxo anterior
		// (cinematica, splash skip, troca de mapa) tenha resetado a flag.
		PC->bShowMouseCursor = true;
		DF_LOG(Log, "[DF|MainMenu|HUD] ShowSaveSlotLayer: modo=%u primeiraConstrucao=%s Z=%d widget=%s",
			static_cast<uint32>(Mode),
			bFirstConstruction ? TEXT("sim") : TEXT("nao"),
			DFMainMenuUI::ViewportZ_SaveSlot,
			SaveSlotLayer ? *SaveSlotLayer->GetClass()->GetName() : TEXT("null"));
	}
}

void ADFMainMenuHUD::HideSaveSlotLayer()
{
	if (SaveSlotLayer)
	{
		DF_LOG(Log, "[DF|MainMenu|HUD] HideSaveSlotLayer: removendo %s",
			*SaveSlotLayer->GetClass()->GetName());
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
		Credits->AddToViewport(DFMainMenuUI::ViewportZ_Credits);
	}
}

void ADFMainMenuHUD::ShowConfirmDialog(UDFConfirmDialogUserWidget* const Inst)
{
	if (Inst)
	{
		Inst->AddToViewport(DFMainMenuUI::ViewportZ_ConfirmDialog);
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
	DFPrepareWidgetForUIModeFocus(Main);
	Mode.SetWidgetToFocus(Main->TakeWidget());
	PC->SetInputMode(Mode);
	PC->bShowMouseCursor = true;
	Main->RefreshForCurrentSaveState();
}

bool ADFMainMenuHUD::SuppressUnderlyingMenuForClassSelectionWorldPreview(const bool bSuppress)
{
	if (bSuppress)
	{
		if (bDetachedMenusForWorldClassSelection)
		{
			DF_LOG(Log, "[DF|MainMenu|HUD] SuppressUnderlyingMenuForClassSelectionWorldPreview: já solto, ignora");
			return false;
		}
		bHadMainInViewportBeforeWorldClassSelection = Main && Main->IsInViewport();
		bHadSaveSlotInViewportBeforeWorldClassSelection = SaveSlotLayer && SaveSlotLayer->IsInViewport();
		if (!bHadMainInViewportBeforeWorldClassSelection && !bHadSaveSlotInViewportBeforeWorldClassSelection)
		{
			DF_LOG(Log, "[DF|MainMenu|HUD] SuppressUnderlyingMenuForClassSelectionWorldPreview: nada no viewport");
			return false;
		}
		if (bHadMainInViewportBeforeWorldClassSelection)
		{
			Main->RemoveFromParent();
		}
		if (bHadSaveSlotInViewportBeforeWorldClassSelection)
		{
			SaveSlotLayer->RemoveFromParent();
		}
		bDetachedMenusForWorldClassSelection = true;
		DF_LOG(Log, "[DF|MainMenu|HUD] SuppressUnderlyingMenuForClassSelectionWorldPreview: RemoveFromParent Main=%s SaveSlot=%s",
			bHadMainInViewportBeforeWorldClassSelection ? TEXT("sim") : TEXT("nao"),
			bHadSaveSlotInViewportBeforeWorldClassSelection ? TEXT("sim") : TEXT("nao"));
		return true;
	}
	if (!bDetachedMenusForWorldClassSelection)
	{
		return false;
	}
	// Z menor primeiro: Main (5) debaixo de slots (20).
	if (bHadMainInViewportBeforeWorldClassSelection && Main)
	{
		Main->AddToViewport(DFMainMenuUI::ViewportZ_Main);
		Main->RefreshForCurrentSaveState();
	}
	if (bHadSaveSlotInViewportBeforeWorldClassSelection && SaveSlotLayer)
	{
		SaveSlotLayer->AddToViewport(DFMainMenuUI::ViewportZ_SaveSlot);
		SaveSlotLayer->RefreshFromSubsystem();
	}
	bDetachedMenusForWorldClassSelection = false;
	bHadMainInViewportBeforeWorldClassSelection = false;
	bHadSaveSlotInViewportBeforeWorldClassSelection = false;
	DF_LOG(Log, "[DF|MainMenu|HUD] SuppressUnderlyingMenuForClassSelectionWorldPreview: reposto no viewport");
	return true;
}

void ADFMainMenuHUD::RestoreFocusAfterClassSelectionWorldPreview()
{
	APlayerController* const PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}
	FInputModeUIOnly Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (SaveSlotLayer && SaveSlotLayer->IsInViewport())
	{
		DFPrepareWidgetForUIModeFocus(SaveSlotLayer);
		Mode.SetWidgetToFocus(SaveSlotLayer->TakeWidget());
	}
	else if (Main && Main->IsInViewport())
	{
		DFPrepareWidgetForUIModeFocus(Main);
		Mode.SetWidgetToFocus(Main->TakeWidget());
	}
	PC->SetInputMode(Mode);
	PC->bShowMouseCursor = true;
}
