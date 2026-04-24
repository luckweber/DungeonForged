// Source/DungeonForged/Private/UI/Minimap/UDFMinimapIconWidget.cpp
#include "UI/Minimap/UDFMinimapIconWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UDFMinimapIconWidget::BindToRoom(ADFMinimapRoom* const InRoom, UTexture2D* const IconTex, const FLinearColor Tint)
{
	Room = InRoom;
	CachedTint = Tint;
	if (RoomIcon)
	{
		if (IconTex)
		{
			RoomIcon->SetBrushFromTexture(IconTex, false);
		}
		RoomIcon->SetColorAndOpacity(CachedTint);
	}
	if (RoomLabel && InRoom)
	{
		const FText Txt = InRoom->RoomDisplayName.IsEmpty() ? FText::GetEmpty() : InRoom->RoomDisplayName;
		RoomLabel->SetText(Txt);
	}
}

void UDFMinimapIconWidget::SetIconState(const EUDFMinimapIconState InState, const bool bVisited, const bool bPulsing)
{
	State = InState;
	bVisitedLocal = bVisited;
	bPulsingLocal = bPulsing;

	ESlateVisibility V = ESlateVisibility::Collapsed;
	if (InState == EUDFMinimapIconState::Lookahead)
	{
		V = ESlateVisibility::HitTestInvisible;
		if (RoomIcon)
		{
			const float Dim = 0.35f;
			RoomIcon->SetColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f, Dim));
		}
	}
	else if (InState == EUDFMinimapIconState::Revealed)
	{
		V = ESlateVisibility::HitTestInvisible;
		// Pulsing current-room: leave alpha/color to `NativeTick` (do not clobber every frame from parent).
		if (RoomIcon && !bPulsing)
		{
			FLinearColor C = CachedTint;
			C.A = 1.f;
			RoomIcon->SetColorAndOpacity(C);
		}
	}

	SetVisibility(V);

	if (VisitedOverlay)
	{
		VisitedOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
		VisitedOverlay->SetRenderOpacity(bVisited ? 0.0f : 0.55f);
	}
}

void UDFMinimapIconWidget::SetLabelExpanded(const bool bShowLabels, const FText& Label)
{
	if (RoomLabel)
	{
		RoomLabel->SetText(Label);
		RoomLabel->SetVisibility(
			bShowLabels && !Label.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UDFMinimapIconWidget::StepPulse(const float DTM)
{
	if (!bPulsingLocal || !RoomIcon || State != EUDFMinimapIconState::Revealed)
	{
		return;
	}
	PulsePhase += DTM * 4.5f;
	const float S = 0.5f + 0.5f * FMath::Sin(PulsePhase);
	const FLinearColor C = RoomIcon->GetColorAndOpacity();
	RoomIcon->SetColorAndOpacity(FLinearColor(C.R, C.G, C.B, 0.65f + 0.35f * S));
}
