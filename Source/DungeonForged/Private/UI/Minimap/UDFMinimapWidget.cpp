// Source/DungeonForged/Private/UI/Minimap/UDFMinimapWidget.cpp
#include "UI/Minimap/UDFMinimapWidget.h"
#include "ADFDungeonManager.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SizeBox.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UI/Minimap/UDFMinimapCaptureComponent.h"

void UDFMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CurrentMapEdgePx = CollapsedMapEdgePx;
	bTargetExpanded = bIsExpanded;
	if (MinimapSizeBox)
	{
		MinimapSizeBox->SetWidthOverride(CurrentMapEdgePx);
		MinimapSizeBox->SetHeightOverride(CurrentMapEdgePx);
	}

	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFDungeonManager* const DM = GI->GetSubsystem<UDFDungeonManager>())
		{
			SubscribedDungeonManager = DM;
			DM->OnRoomRevealed.AddDynamic(this, &UDFMinimapWidget::OnRoomRevealed);
			DM->OnRoomVisited.AddDynamic(this, &UDFMinimapWidget::OnRoomVisited);
			DM->OnPlayerMinimapRoomChanged.AddDynamic(this, &UDFMinimapWidget::OnPlayerMinimapRoomChanged);
		}
	}
	RebuildKnownRoomsList();
	RefreshAllRoomIcons();
	UpdateMinimapTexture();
	ApplyZoomToCapture();
}

void UDFMinimapWidget::NativeDestruct()
{
	if (SubscribedDungeonManager.IsValid())
	{
		if (UDFDungeonManager* const DM = SubscribedDungeonManager.Get())
		{
			DM->OnRoomRevealed.RemoveDynamic(this, &UDFMinimapWidget::OnRoomRevealed);
			DM->OnRoomVisited.RemoveDynamic(this, &UDFMinimapWidget::OnRoomVisited);
			DM->OnPlayerMinimapRoomChanged.RemoveDynamic(this, &UDFMinimapWidget::OnPlayerMinimapRoomChanged);
		}
	}
	Super::NativeDestruct();
}

void UDFMinimapWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateExpansionLayout(InDeltaTime);
	if (ADFPlayerCharacter* const C = GetDFPlayerCharacter())
	{
		UpdatePlayerRotation(C->GetActorRotation().Yaw);
	}
	UpdateIconPositionsOnly();
	if (SubscribedDungeonManager.IsValid())
	{
		if (UDFDungeonManager* const DM = SubscribedDungeonManager.Get())
		{
			if (ADFMinimapRoom* const Cur = DM->GetPlayerCurrentMinimapRoom())
			{
				if (TObjectPtr<UDFMinimapIconWidget>* const Ptr = RoomToIcon.Find(Cur))
				{
					if (UDFMinimapIconWidget* const Ic = Ptr->Get())
					{
						Ic->StepPulse(InDeltaTime);
					}
				}
			}
		}
	}
	UpdateMinimapTexture();
}

void UDFMinimapWidget::RebuildKnownRoomsList()
{
	KnownRooms.Reset();
	if (SubscribedDungeonManager.IsValid())
	{
		if (const UDFDungeonManager* const DM = SubscribedDungeonManager.Get())
		{
			for (const TObjectPtr<ADFMinimapRoom>& R : DM->RegisteredMinimapRooms)
			{
				if (R)
				{
					KnownRooms.Add(R.Get());
				}
			}
		}
	}
}

void UDFMinimapWidget::OnRoomRevealed(ADFMinimapRoom* const Room)
{
	RebuildKnownRoomsList();
	RefreshAllRoomIcons();
}

void UDFMinimapWidget::OnRoomVisited(ADFMinimapRoom* const Room)
{
	(void)Room;
	RefreshAllRoomIcons();
}

void UDFMinimapWidget::OnPlayerMinimapRoomChanged(ADFMinimapRoom* const Room)
{
	// Pulse updated in RefreshAllRoomIcons; leave Room unused or use for one-shot.
	(void)Room;
	RefreshAllRoomIcons();
}

void UDFMinimapWidget::UpdateMinimapTexture()
{
	if (!MinimapTexture)
	{
		return;
	}
	UTextureRenderTarget2D* RT = nullptr;
	if (MinimapCaptureComponent)
	{
		RT = MinimapCaptureComponent->MinimapRenderTarget;
	}
	if (RT)
	{
		// UE 5.4 UImage: use resource object; render targets are valid Slate brush resources.
		MinimapTexture->SetBrushResourceObject(RT);
	}
}

void UDFMinimapWidget::UpdatePlayerRotation(const float YawDegrees)
{
	if (!PlayerDot)
	{
		return;
	}
	PlayerDot->SetRenderTransformAngle(FMath::UnwindDegrees(YawDegrees));
}

void UDFMinimapWidget::SetMinimapExpanded(const bool bExpanded)
{
	bTargetExpanded = bExpanded;
}

FLinearColor UDFMinimapWidget::GetTintForRoomType(const EDFRoomType Type)
{
	switch (Type)
	{
		case EDFRoomType::Normal: return FLinearColor(0.5f, 0.5f, 0.5f, 1.f);
		case EDFRoomType::Elite: return FLinearColor(1.0f, 0.55f, 0.0f, 1.f);
		case EDFRoomType::Boss: return FLinearColor(0.9f, 0.1f, 0.1f, 1.f);
		case EDFRoomType::Treasure: return FLinearColor(1.0f, 0.95f, 0.0f, 1.f);
		case EDFRoomType::Merchant: return FLinearColor(0.0f, 0.7f, 0.2f, 1.f);
		case EDFRoomType::Start: return FLinearColor(0.3f, 0.5f, 1.0f, 1.f);
		case EDFRoomType::Exit: return FLinearColor(0.5f, 0.0f, 0.5f, 1.f);
		default: return FLinearColor::White;
	}
}

UTexture2D* UDFMinimapWidget::ResolveIconTexture(ADFMinimapRoom* const Room) const
{
	if (!Room)
	{
		return nullptr;
	}
	if (UTexture2D* const FromRoom = Room->GetIconTexture())
	{
		return FromRoom;
	}
	if (const TObjectPtr<UTexture2D>* P = DefaultRoomIconsByType.Find(Room->RoomType))
	{
		return *P;
	}
	return nullptr;
}

void UDFMinimapWidget::SetZoomLevel(const float InZoom)
{
	ZoomLevel = FMath::Max(0.1f, InZoom);
	ApplyZoomToCapture();
}

void UDFMinimapWidget::ApplyZoomToCapture()
{
	if (!MinimapCaptureComponent)
	{
		return;
	}
	MinimapCaptureComponent->SetOrthoWidth(MinimapCaptureComponent->BaseOrthoWidth / ZoomLevel);
}

EUDFMinimapIconState UDFMinimapWidget::ComputeState(ADFMinimapRoom* const Room) const
{
	if (!Room)
	{
		return EUDFMinimapIconState::Hidden;
	}
	if (Room->bIsRevealed)
	{
		return EUDFMinimapIconState::Revealed;
	}
	if (Room->IsAdjacentToRevealed())
	{
		return EUDFMinimapIconState::Lookahead;
	}
	return EUDFMinimapIconState::Hidden;
}

void UDFMinimapWidget::UpdateExpansionLayout(const float DeltaTime)
{
	const float Target = bTargetExpanded ? ExpandedMapEdgePx : CollapsedMapEdgePx;
	CurrentMapEdgePx = FMath::FInterpTo(CurrentMapEdgePx, Target, DeltaTime, 8.f);
	if (FMath::IsNearlyEqual(CurrentMapEdgePx, Target, 0.5f))
	{
		CurrentMapEdgePx = Target;
		bIsExpanded = bTargetExpanded;
	}
	if (MinimapSizeBox)
	{
		MinimapSizeBox->SetWidthOverride(CurrentMapEdgePx);
		MinimapSizeBox->SetHeightOverride(CurrentMapEdgePx);
	}
	const bool bShowLabels = bTargetExpanded && (CurrentMapEdgePx > 0.5f * (CollapsedMapEdgePx + ExpandedMapEdgePx));
	for (auto& Pair : RoomToIcon)
	{
		if (Pair.Value)
		{
			if (Pair.Key && bShowLabels)
			{
				Pair.Value->SetLabelExpanded(
					true,
					Pair.Key->RoomDisplayName.IsEmpty() ? FText::GetEmpty() : Pair.Key->RoomDisplayName);
			}
			else
			{
				Pair.Value->SetLabelExpanded(false, FText::GetEmpty());
			}
		}
	}
}

void UDFMinimapWidget::UpdateIconPositionsOnly()
{
	for (const TPair<ADFMinimapRoom*, TObjectPtr<UDFMinimapIconWidget>>& Pair : RoomToIcon)
	{
		ADFMinimapRoom* R = Pair.Key;
		UDFMinimapIconWidget* const Icon = Pair.Value;
		if (R && Icon && ComputeState(R) != EUDFMinimapIconState::Hidden)
		{
			PositionIcon(R, Icon);
		}
	}
}

void UDFMinimapWidget::PositionIcon(ADFMinimapRoom* const Room, UDFMinimapIconWidget* const Icon) const
{
	if (!IconLayer || !Room || !Icon)
	{
		return;
	}
	ADFPlayerCharacter* const C = GetDFPlayerCharacter();
	if (!C)
	{
		return;
	}
	const FVector PLoc = C->GetActorLocation();
	const FVector& RPos = Room->RoomCenter;
	const FVector2D D(RPos.X - PLoc.X, RPos.Y - PLoc.Y);
	float OrthoW = 3000.f;
	if (MinimapCaptureComponent)
	{
		if (USceneCaptureComponent2D* const Cap = MinimapCaptureComponent->GetCapture())
		{
			OrthoW = FMath::Max(10.f, Cap->OrthoWidth);
		}
	}
	else
	{
		OrthoW = 3000.f;
	}
	const FVector2D PanelSize = IconLayer->GetCachedGeometry().GetLocalSize();
	if (PanelSize.IsNearlyZero())
	{
		return;
	}
	const float Scale = (FMath::Min(PanelSize.X, PanelSize.Y)) / OrthoW;
	const FVector2D OffsetPixels(D.X * Scale, -D.Y * Scale);
	if (UCanvasPanelSlot* const CanvasPanelSlot = Cast<UCanvasPanelSlot>(Icon->Slot))
	{
		CanvasPanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		CanvasPanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasPanelSlot->SetPosition(OffsetPixels);
	}
}

void UDFMinimapWidget::RefreshAllRoomIcons()
{
	if (!IconLayer)
	{
		return;
	}
	UDFDungeonManager* const DM = SubscribedDungeonManager.Get();
	ADFMinimapRoom* const Current = DM ? DM->GetPlayerCurrentMinimapRoom() : nullptr;
	TSubclassOf<UDFMinimapIconWidget> Cls(UDFMinimapIconWidget::StaticClass());
	if (IconWidgetClass)
	{
		Cls = IconWidgetClass;
	}
	for (ADFMinimapRoom* R : KnownRooms)
	{
		if (!IsValid(R))
		{
			continue;
		}
		const EUDFMinimapIconState S = ComputeState(R);
		if (S == EUDFMinimapIconState::Hidden)
		{
			if (TObjectPtr<UDFMinimapIconWidget>* const Ptr = RoomToIcon.Find(R))
			{
				if (UDFMinimapIconWidget* const Ic = Ptr->Get())
				{
					Ic->SetIconState(S, R->bIsVisited, false);
					Ic->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
			continue;
		}
		UTexture2D* const Tex = ResolveIconTexture(R);
		UDFMinimapIconWidget* Icon = nullptr;
		if (TObjectPtr<UDFMinimapIconWidget>* const Found = RoomToIcon.Find(R))
		{
			Icon = Found->Get();
		}
		if (!Icon)
		{
			Icon = CreateWidget<UDFMinimapIconWidget>(this, Cls);
			if (!Icon)
			{
				continue;
			}
			IconLayer->AddChild(Icon);
			if (UCanvasPanelSlot* const IconCanvasSlot = Cast<UCanvasPanelSlot>(Icon->Slot))
			{
				IconCanvasSlot->SetZOrder(2);
			}
			RoomToIcon.Add(R, Icon);
		}
		Icon->BindToRoom(R, Tex, GetTintForRoomType(R->RoomType));
		const bool bPulse = (R == Current) && (S == EUDFMinimapIconState::Revealed);
		Icon->SetIconState(S, R->bIsVisited, bPulse);
		Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		PositionIcon(R, Icon);
	}
}
