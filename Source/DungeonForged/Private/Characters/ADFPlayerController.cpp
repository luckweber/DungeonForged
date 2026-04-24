// Source/DungeonForged/Private/Characters/ADFPlayerController.cpp

#include "Characters/ADFPlayerController.h"
#include "Debug/UDFCheatManager.h"
#include "Debug/UDFGASDebugOverlayWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

ADFPlayerController::ADFPlayerController()
{
	CheatClass = UDFCheatManager::StaticClass();
}

void ADFPlayerController::ToggleGASDebugOverlay()
{
#if !UE_BUILD_SHIPPING
	if (!GASDebugOverlay)
	{
		GASDebugOverlay = CreateWidget<UDFGASDebugOverlayWidget>(this, UDFGASDebugOverlayWidget::StaticClass());
		if (GASDebugOverlay)
		{
			GASDebugOverlay->AddToViewport(2000);
		}
	}
	if (GASDebugOverlay)
	{
		bGASDebugOverlayVisible = !bGASDebugOverlayVisible;
		GASDebugOverlay->SetVisibility(
			bGASDebugOverlayVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
#endif
}
