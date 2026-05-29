// Source/DungeonForged/Private/UI/UDFRunHUDRootWidget.cpp
#include "UI/UDFRunHUDRootWidget.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"

void UDFRunHUDRootWidget::MountLayerWidget(UUserWidget* const Child, const int32 ZOrder)
{
	if (!Child)
	{
		return;
	}
	if (UOverlay* const Root = HUDRootOverlay)
	{
		Child->RemoveFromParent();
		if (UOverlaySlot* const OverlaySlot = Root->AddChildToOverlay(Child))
		{
			OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			OverlaySlot->SetVerticalAlignment(VAlign_Fill);
			OverlaySlot->SetPadding(FMargin(0.f));
		}
		return;
	}
	Child->AddToViewport(ZOrder);
}

void UDFRunHUDRootWidget::SetLayerVisibility(UUserWidget* const Child, const ESlateVisibility InVisibility)
{
	if (Child)
	{
		Child->SetVisibility(InVisibility);
	}
}
