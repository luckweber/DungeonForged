// Source/DungeonForged/Private/Equipment/UDFEquipmentSlotWidget.cpp
#include "Equipment/UDFEquipmentSlotWidget.h"
#include "Equipment/UDFEquipmentComponent.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Data/DFDataTableStructs.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Merchant/DFMerchantData.h"
#include "Equipment/UDFItemDragDropOperation.h"
#include "Input/Events.h"

namespace
{
constexpr FLinearColor EquipDropAllowedBorder(0.15f, 0.9f, 0.35f, 1.f);
constexpr FLinearColor EquipDropDeniedBorder(0.95f, 0.15f, 0.1f, 1.f);

/** @return -1 = not an equippable DF bag drag, 0 = blocked, 1 = allowed */
int32 EvaluateEquipmentSlotHoverVerdict(
	const ADFPlayerCharacter* Player,
	const UDFItemDragDropOperation* Payload,
	const EEquipmentSlot TargetSlot)
{
	if (!Player || !Payload || TargetSlot == EEquipmentSlot::None)
	{
		return -1;
	}
	if (Payload->DragOrigin == EDFItemDragOrigin::EquipmentSlot)
	{
		return 0;
	}
	const bool bFromBag =
		Payload->DragOrigin == EDFItemDragOrigin::BagSlot ||
		(Payload->DragOrigin == EDFItemDragOrigin::None && !Payload->bFromEquipmentSlot);
	if (!bFromBag)
	{
		return -1;
	}
	if (Payload->ItemRowName.IsNone())
	{
		return 0;
	}
	UDFEquipmentComponent* const Eq = Player->GetDFEquipment();
	if (!Eq)
	{
		return 0;
	}
	FString Err;
	return Eq->PredictCanEquipItem(Payload->ItemRowName, TargetSlot, Err, Payload->SourceInventorySlotIndex)
			   ? 1
			   : 0;
}

// Shared helper that resets the slot widgets to their empty visual state.
void ClearEquipSlotVisuals(UImage* SlotIcon, UTextBlock* ItemLevelText, UImage* RarityBorder)
{
	if (SlotIcon)
	{
		SlotIcon->SetBrushFromTexture(nullptr, false);
		SlotIcon->SetColorAndOpacity(FLinearColor(0.35f, 0.35f, 0.35f, 0.9f));
	}
	if (ItemLevelText)
	{
		ItemLevelText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (RarityBorder)
	{
		RarityBorder->SetColorAndOpacity(FLinearColor(0.2f, 0.2f, 0.2f, 0.7f));
		RarityBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

UImage* CreateIconDragPreview(UObject* Outer, UTexture2D* Texture, const FVector2D& DesiredSize)
{
	UImage* const Img = NewObject<UImage>(Outer);
	if (!Img)
	{
		return nullptr;
	}
	Img->SetVisibility(ESlateVisibility::HitTestInvisible);
	Img->SetColorAndOpacity(FLinearColor::White);
	Img->SetDesiredSizeOverride(DesiredSize);
	if (Texture)
	{
		Img->SetBrushFromTexture(Texture, false);
	}
	return Img;
}
} // namespace

void UDFEquipmentSlotWidget::SetEquipmentSlot(EEquipmentSlot InSlot)
{
	EquipSlot = InSlot;
	RefreshFromEquipment();
}

void UDFEquipmentSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RegisterOwningPlayerPossessListener();
	BindEquipmentChangedDelegate();
	RefreshFromEquipment();
}

void UDFEquipmentSlotWidget::NativeDestruct()
{
	UnbindEquipmentChangedDelegate();
	UnregisterOwningPlayerPossessListener();
	Super::NativeDestruct();
}

void UDFEquipmentSlotWidget::RegisterOwningPlayerPossessListener()
{
	UnregisterOwningPlayerPossessListener();
	if (APlayerController* PC = GetOwningPlayer())
	{
		OwningPossessPcWeak = PC;
		PC->OnPossessedPawnChanged.AddDynamic(
			this, &UDFEquipmentSlotWidget::HandleOwningPlayerPossessedChanged);
	}
}

void UDFEquipmentSlotWidget::UnregisterOwningPlayerPossessListener()
{
	if (OwningPossessPcWeak.IsValid())
	{
		if (APlayerController* PC = OwningPossessPcWeak.Get())
		{
			PC->OnPossessedPawnChanged.RemoveDynamic(
				this, &UDFEquipmentSlotWidget::HandleOwningPlayerPossessedChanged);
		}
	}
	OwningPossessPcWeak.Reset();
}

void UDFEquipmentSlotWidget::BindEquipmentChangedDelegate()
{
	UnbindEquipmentChangedDelegate();

	ADFPlayerCharacter* PC = GetDFPlayerCharacter();
	UDFEquipmentComponent* Eq = PC ? PC->GetDFEquipment() : nullptr;
	if (Eq && IsValid(Eq))
	{
		Eq->OnEquipmentChanged.AddDynamic(this, &UDFEquipmentSlotWidget::HandleEquipmentChangedBroadcast);
		BoundEquipmentWeak = Eq;
	}
}

void UDFEquipmentSlotWidget::UnbindEquipmentChangedDelegate()
{
	if (UDFEquipmentComponent* Eq = BoundEquipmentWeak.Get())
	{
		Eq->OnEquipmentChanged.RemoveDynamic(this, &UDFEquipmentSlotWidget::HandleEquipmentChangedBroadcast);
	}
	BoundEquipmentWeak.Reset();
}

void UDFEquipmentSlotWidget::HandleOwningPlayerPossessedChanged(APawn* PreviousPawn, APawn* NewPawn)
{
	(void)PreviousPawn;
	(void)NewPawn;

	BindEquipmentChangedDelegate();
	RefreshFromEquipment();
}

void UDFEquipmentSlotWidget::HandleEquipmentChangedBroadcast(const EEquipmentSlot ChangedEquipSlot, const FName ItemRow)
{
	(void)ChangedEquipSlot;
	(void)ItemRow;
	RefreshFromEquipment();
}

void UDFEquipmentSlotWidget::RefreshFromEquipment()
{
	if (EquipSlot == EEquipmentSlot::None)
	{
		return;
	}

	const ADFPlayerCharacter* const PC = GetDFPlayerCharacter();
	if (!PC)
	{
		ClearEquipSlotVisuals(SlotIcon, ItemLevelText, RarityBorder);
		return;
	}

	const UDFEquipmentComponent* const Eq = PC->GetDFEquipment();
	if (!Eq)
	{
		ClearEquipSlotVisuals(SlotIcon, ItemLevelText, RarityBorder);
		return;
	}

	const FDFItemTableRow* const Row = Eq->GetEquippedItemDataRaw(EquipSlot);
	if (!Row)
	{
		ClearEquipSlotVisuals(SlotIcon, ItemLevelText, RarityBorder);
		return;
	}

	// ── Icon ─────────────────────────────────────────────────────────────────
	// BUG FIX: original code only wrote to SlotIcon when Row->Icon != nullptr.
	// When the data row existed but Icon was null the stale previous texture
	// stayed visible. Now we always update so the widget state is consistent.
	if (SlotIcon)
	{
		if (Row->Icon)
		{
			SlotIcon->SetBrushFromTexture(Row->Icon, true);
			SlotIcon->SetColorAndOpacity(FLinearColor::White);
		}
		else
		{
			SlotIcon->SetBrushFromTexture(nullptr, false);
			SlotIcon->SetColorAndOpacity(FLinearColor(0.35f, 0.35f, 0.35f, 0.9f));
		}
	}

	// ── Item level ────────────────────────────────────────────────────────────
	if (ItemLevelText)
	{
		if (Row->ItemLevel > 0)
		{
			ItemLevelText->SetText(FText::AsNumber(Row->ItemLevel));
			ItemLevelText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			ItemLevelText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// ── Rarity border ─────────────────────────────────────────────────────────
	if (RarityBorder)
	{
		RarityBorder->SetColorAndOpacity(UDFMerchantUIStatics::RarityToColor(Row->Rarity));
		RarityBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

bool UDFEquipmentSlotWidget::RequestEquipItemRow(
	const FName ItemRowName,
	const int32 SourceBagSlotIndex)
{
	ADFPlayerCharacter* const PC = GetDFPlayerCharacter();
	if (!PC || ItemRowName.IsNone() || EquipSlot == EEquipmentSlot::None)
	{
		return false;
	}
	if (UDFEquipmentComponent* const Eq = PC->GetDFEquipment())
	{
		return Eq->EquipItem(ItemRowName, EquipSlot, SourceBagSlotIndex);
	}
	return false;
}

FReply UDFEquipmentSlotWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const ADFPlayerCharacter* const PC = GetDFPlayerCharacter();
		const UDFEquipmentComponent* const Eq = PC ? PC->GetDFEquipment() : nullptr;
		if (Eq && EquipSlot != EEquipmentSlot::None && !Eq->IsSlotEmpty(EquipSlot))
		{
			return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
		}
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UDFEquipmentSlotWidget::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	OutOperation = nullptr;
	const ADFPlayerCharacter* const PC = GetDFPlayerCharacter();
	const UDFEquipmentComponent* const Eq = PC ? PC->GetDFEquipment() : nullptr;
	if (!PC || !Eq || EquipSlot == EEquipmentSlot::None || Eq->IsSlotEmpty(EquipSlot))
	{
		return;
	}

	const FName RowName = Eq->EquippedItems.FindRef(EquipSlot);
	if (RowName.IsNone())
	{
		return;
	}

	FVector2D IconSize(48.f, 48.f);
	if (SlotIcon)
	{
		const FVector2D Measured = SlotIcon->GetCachedGeometry().GetLocalSize();
		if (Measured.X > 0.f && Measured.Y > 0.f)
		{
			IconSize = Measured;
		}
	}
	IconSize *= FMath::Max(1.f, DragPreviewIconScale);
	IconSize.X = FMath::Max(DragPreviewMinimumSize.X, IconSize.X);
	IconSize.Y = FMath::Max(DragPreviewMinimumSize.Y, IconSize.Y);

	UDFItemDragDropOperation* const Op = NewObject<UDFItemDragDropOperation>(this);
	Op->DragOrigin               = EDFItemDragOrigin::EquipmentSlot;
	Op->ItemRowName              = RowName;
	Op->SourceInventorySlotIndex = INDEX_NONE;
	Op->bFromEquipmentSlot       = true;
	Op->SourceEquipmentSlot      = EquipSlot;
	if (const FDFItemTableRow* Row = Eq->GetEquippedItemDataRaw(EquipSlot))
	{
		Op->DefaultDragVisual = CreateIconDragPreview(Op, Row->Icon, IconSize);
	}
	Op->Pivot             = EDragPivot::MouseDown;
	OutOperation          = Op;
}

void UDFEquipmentSlotWidget::UpdateEquipSlotDragHoverVisualFromOperation(UDragDropOperation* const InOperation)
{
	if (!RarityBorder)
	{
		return;
	}
	const UDFItemDragDropOperation* Payload = Cast<UDFItemDragDropOperation>(InOperation);
	const int32 Verdict                     = EvaluateEquipmentSlotHoverVerdict(GetDFPlayerCharacter(), Payload, EquipSlot);
	if (Verdict < 0)
	{
		RefreshFromEquipment();
		return;
	}
	RarityBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	RarityBorder->SetColorAndOpacity(Verdict == 1 ? EquipDropAllowedBorder : EquipDropDeniedBorder);
}

void UDFEquipmentSlotWidget::NativeOnDragEnter(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);
	UpdateEquipSlotDragHoverVisualFromOperation(InOperation);
}

void UDFEquipmentSlotWidget::NativeOnDragLeave(
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	RefreshFromEquipment();
}

bool UDFEquipmentSlotWidget::NativeOnDragOver(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	UpdateEquipSlotDragHoverVisualFromOperation(InOperation);
	if (Cast<UDFItemDragDropOperation>(InOperation))
	{
		return true;
	}
	return Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);
}

bool UDFEquipmentSlotWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	const UDFItemDragDropOperation* const P = Cast<UDFItemDragDropOperation>(InOperation);
	bool bCommitted                           = false;
	do
	{
		if (!P)
		{
			break;
		}
		if (P->DragOrigin == EDFItemDragOrigin::EquipmentSlot)
		{
			break;
		}
		const bool bFromBag =
			P->DragOrigin == EDFItemDragOrigin::BagSlot ||
			(P->DragOrigin == EDFItemDragOrigin::None && !P->bFromEquipmentSlot);
		if (!bFromBag)
		{
			break;
		}
		if (P->ItemRowName.IsNone() || EquipSlot == EEquipmentSlot::None)
		{
			break;
		}
		const int32 SrcBag = P->SourceInventorySlotIndex;
		bCommitted         = RequestEquipItemRow(P->ItemRowName, SrcBag);
	} while (false);

	RefreshFromEquipment();
	return bCommitted;
}