// Source/DungeonForged/Private/UI/UDFLockOnWidget.cpp

#include "UI/UDFLockOnWidget.h"
#include "GameFramework/PlayerController.h"

void UDFLockOnWidget::UpdateScreenPosition(APlayerController* const PC, const FVector WorldPos)
{
	if (!PC)
	{
		return;
	}
	FVector2D Screen;
	if (PC->ProjectWorldLocationToScreen(WorldPos, Screen, false))
	{
		SetPositionInViewport(Screen, true);
	}
}
