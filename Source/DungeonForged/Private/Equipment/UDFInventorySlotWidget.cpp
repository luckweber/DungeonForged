// Source/DungeonForged/Private/Equipment/UDFInventorySlotWidget.cpp
#include "Equipment/UDFInventorySlotWidget.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Data/DFDataTableStructs.h"
#include "DFInventoryComponent.h"
#include "Equipment/UDFEquipmentComponent.h"
#include "Equipment/UDFItemDragDropOperation.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Input/Events.h"
#include "Merchant/DFMerchantData.h"

namespace
{
constexpr FLinearColor BagDropAllowedBorder(0.15f, 0.9f, 0.35f, 1.f);
constexpr FLinearColor BagDropDeniedBorder(0.95f, 0.15f, 0.1f, 1.f);

/** @return -1 = not DF item drag, 0 = blocked, 1 = allowed */
int32 EvaluateBagHoverVerdict(const ADFPlayerCharacter* Player, const UDFItemDragDropOperation* Payload, const int32 BagIndex)
{
	if (!Player || !Payload || !Player->GetDFInventory() || BagIndex == INDEX_NONE ||
		!Player->GetDFInventory()->IsSlotValidIndex(BagIndex))
	{
		return -1;
	}

	const UDFInventoryComponent* const Inv = Player->GetDFInventory();

	const EDFItemDragOrigin Ori = Payload->DragOrigin;
	const bool bUnequipFromEquipment = Ori == EDFItemDragOrigin::EquipmentSlot ||
		(Ori == EDFItemDragOrigin::None && Payload->bFromEquipmentSlot);

	if (bUnequipFromEquipment)
	{
		const UDFEquipmentComponent* Eq = Player->GetDFEquipment();
		if (!Eq || Payload->SourceEquipmentSlot == EEquipmentSlot::None)
		{
			return 0;
		}
		const FName EquippedRow = Eq->EquippedItems.FindRef(Payload->SourceEquipmentSlot);
		if (EquippedRow.IsNone())
		{
			return 0;
		}
		return Inv->PredictCanReceiveUnequippedStackAtBagIndex(BagIndex, EquippedRow, 1) ? 1 : 0;
	}

	const bool bBagReorder =
		Ori == EDFItemDragOrigin::BagSlot || (Ori == EDFItemDragOrigin::None && !Payload->bFromEquipmentSlot);
	if (bBagReorder && Payload->SourceInventorySlotIndex != INDEX_NONE)
	{
		if (!Inv->IsSlotValidIndex(Payload->SourceInventorySlotIndex))
		{
			return 0;
		}
		if (Payload->SourceInventorySlotIndex == BagIndex)
		{
			return 0;
		}
		return 1;
	}

	return -1;
}

// Reusable helper: clears the slot widgets to their empty visual state.
void ClearSlotVisuals(UImage* SlotIcon, UTextBlock* QuantityText, UImage* RarityBorder)
{
	if (SlotIcon)
	{
		SlotIcon->SetBrushFromTexture(nullptr, false);
		SlotIcon->SetColorAndOpacity(FLinearColor(0.35f, 0.35f, 0.35f, 0.9f));
	}
	if (QuantityText)
	{
		QuantityText->SetVisibility(ESlateVisibility::Collapsed);
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

void UDFInventorySlotWidget::SetBagSlotIndex(const int32 InIndex)
{
	SlotIndex = InIndex;
	RefreshFromInventory();
}

void UDFInventorySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RegisterOwningPlayerPossessListener();
	BindInventoryChangedDelegate();
	RefreshFromInventory();
}

void UDFInventorySlotWidget::NativeDestruct()
{
	UnbindInventoryChangedDelegate();
	UnregisterOwningPlayerPossessListener();
	Super::NativeDestruct();
}

void UDFInventorySlotWidget::RegisterOwningPlayerPossessListener()
{
	UnregisterOwningPlayerPossessListener();
	if (APlayerController* PC = GetOwningPlayer())
	{
		OwningPossessPcWeak = PC;
		PC->OnPossessedPawnChanged.AddDynamic(
			this, &UDFInventorySlotWidget::HandleOwningPlayerPossessedChanged);
	}
}

void UDFInventorySlotWidget::UnregisterOwningPlayerPossessListener()
{
	if (OwningPossessPcWeak.IsValid())
	{
		if (APlayerController* PC = OwningPossessPcWeak.Get())
		{
			PC->OnPossessedPawnChanged.RemoveDynamic(
				this, &UDFInventorySlotWidget::HandleOwningPlayerPossessedChanged);
		}
	}
	OwningPossessPcWeak.Reset();
}

void UDFInventorySlotWidget::BindInventoryChangedDelegate()
{
	UnbindInventoryChangedDelegate();

	ADFPlayerCharacter* PC = GetDFPlayerCharacter();
	UDFInventoryComponent* Inv = PC ? PC->GetDFInventory() : nullptr;
	if (Inv && IsValid(Inv))
	{
		Inv->OnInventoryChanged.AddDynamic(this, &UDFInventorySlotWidget::HandleInventoryChangedBroadcast);
		BoundInventoryWeak = Inv;
	}
}

void UDFInventorySlotWidget::UnbindInventoryChangedDelegate()
{
	if (UDFInventoryComponent* Inv = BoundInventoryWeak.Get())
	{
		Inv->OnInventoryChanged.RemoveDynamic(this, &UDFInventorySlotWidget::HandleInventoryChangedBroadcast);
	}
	BoundInventoryWeak.Reset();
}

void UDFInventorySlotWidget::HandleOwningPlayerPossessedChanged(APawn* PreviousPawn, APawn* NewPawn)
{
	(void)PreviousPawn;
	(void)NewPawn;

	BindInventoryChangedDelegate();
	RefreshFromInventory();
}

void UDFInventorySlotWidget::HandleInventoryChangedBroadcast()
{
	RefreshFromInventory();
}

void UDFInventorySlotWidget::RefreshFromInventory()
{
	const ADFPlayerCharacter* const PC = GetDFPlayerCharacter();
	if (!PC)
	{
		ClearSlotVisuals(SlotIcon, QuantityText, RarityBorder);
		return;
	}

	const UDFInventoryComponent* const Inv = PC->GetDFInventory();
	if (!Inv || !Inv->Items.IsValidIndex(SlotIndex))
	{
		ClearSlotVisuals(SlotIcon, QuantityText, RarityBorder);
		return;
	}

	const FDFInventorySlot& S = Inv->Items[SlotIndex];
	if (S.RowName.IsNone() || S.Quantity < 1)
	{
		ClearSlotVisuals(SlotIcon, QuantityText, RarityBorder);
		return;
	}

	const FDFItemTableRow* const Row = Inv->GetItemData(S.RowName);
	if (!Row)
	{
		ClearSlotVisuals(SlotIcon, QuantityText, RarityBorder);
		return;
	}

	// ── Icon ─────────────────────────────────────────────────────────────────
	// BUG FIX: the original code only set the icon when Row->Icon != nullptr.
	// When Icon was null the previous (stale) texture remained visible.
	// Now we always update SlotIcon so state is always consistent.
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
			SlotIcon->SetColorAndOpacity(FLinearColor(0.007499f, 0.005605f, 0.005182f, 1.f));
		}
	}

	// ── Quantity ──────────────────────────────────────────────────────────────
	if (QuantityText)
	{
		if (S.Quantity > 1)
		{
			QuantityText->SetText(FText::AsNumber(S.Quantity));
			QuantityText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			QuantityText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// ── Rarity border ─────────────────────────────────────────────────────────
	if (RarityBorder)
	{
		RarityBorder->SetColorAndOpacity(UDFMerchantUIStatics::RarityToColor(Row->Rarity));
		RarityBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

bool UDFInventorySlotWidget::HasStackForDrag() const
{
	const ADFPlayerCharacter* const PC = GetDFPlayerCharacter();
	if (!PC)
	{
		return false;
	}
	const UDFInventoryComponent* const Inv = PC->GetDFInventory();
	if (!Inv || !Inv->Items.IsValidIndex(SlotIndex))
	{
		return false;
	}
	const FDFInventorySlot& S = Inv->Items[SlotIndex];
	if (S.RowName.IsNone() || S.Quantity < 1)
	{
		return false;
	}
	return Inv->GetItemData(S.RowName) != nullptr;
}

FReply UDFInventorySlotWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && HasStackForDrag())
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UDFInventorySlotWidget::NativeOnMouseButtonDoubleClick(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		RequestEquipFromThisSlotUsingResolvedEquipmentSlot();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void UDFInventorySlotWidget::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	OutOperation = nullptr;
	if (!HasStackForDrag())
	{
		return;
	}
	ADFPlayerCharacter* const PC = GetDFPlayerCharacter();
	const UDFInventoryComponent* const Inv = PC ? PC->GetDFInventory() : nullptr;
	if (!PC || !Inv || !Inv->Items.IsValidIndex(SlotIndex))
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
	Op->DragOrigin              = EDFItemDragOrigin::BagSlot;
	Op->ItemRowName             = Inv->Items[SlotIndex].RowName;
	Op->SourceInventorySlotIndex = SlotIndex;
	Op->bFromEquipmentSlot      = false;
	Op->SourceEquipmentSlot     = EEquipmentSlot::None;
	if (const FDFItemTableRow* Row = Inv->GetItemData(Inv->Items[SlotIndex].RowName))
	{
		Op->DefaultDragVisual = CreateIconDragPreview(Op, Row->Icon, IconSize);
	}
	Op->Pivot             = EDragPivot::MouseDown;
	OutOperation          = Op;
}

void UDFInventorySlotWidget::UpdateBagSlotDragHoverVisualFromOperation(UDragDropOperation* const InOperation)
{
	if (!RarityBorder)
	{
		return;
	}
	const UDFItemDragDropOperation* Payload = Cast<UDFItemDragDropOperation>(InOperation);
	const int32 Verdict = EvaluateBagHoverVerdict(GetDFPlayerCharacter(), Payload, SlotIndex);
	if (Verdict < 0)
	{
		RefreshFromInventory();
		return;
	}
	RarityBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	RarityBorder->SetColorAndOpacity(Verdict == 1 ? BagDropAllowedBorder : BagDropDeniedBorder);
}

void UDFInventorySlotWidget::NativeOnDragEnter(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);
	UpdateBagSlotDragHoverVisualFromOperation(InOperation);
}

void UDFInventorySlotWidget::NativeOnDragLeave(
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	RefreshFromInventory();
}

bool UDFInventorySlotWidget::NativeOnDragOver(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	UpdateBagSlotDragHoverVisualFromOperation(InOperation);
	if (Cast<UDFItemDragDropOperation>(InOperation))
	{
		return true;
	}
	return Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);
}

bool UDFInventorySlotWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	const UDFItemDragDropOperation* const P = Cast<UDFItemDragDropOperation>(InOperation);
	bool bCommitted = false;
	do
	{
		if (!P)
		{
			break;
		}
		ADFPlayerCharacter* const PC = GetDFPlayerCharacter();
		if (!PC)
		{
			break;
		}

		const EDFItemDragOrigin Ori = P->DragOrigin;
		const bool bUnequipFromEquipment =
			Ori == EDFItemDragOrigin::EquipmentSlot ||
			(Ori == EDFItemDragOrigin::None && P->bFromEquipmentSlot);

		if (bUnequipFromEquipment)
		{
			if (P->SourceEquipmentSlot == EEquipmentSlot::None)
			{
				break;
			}
			UDFInventoryComponent* const Inv = PC->GetDFInventory();
			if (!Inv || !Inv->IsSlotValidIndex(SlotIndex))
			{
				break;
			}
			if (UDFEquipmentComponent* const Eq = PC->GetDFEquipment())
			{
				Eq->RequestUnequipToBagSlot(P->SourceEquipmentSlot, SlotIndex);
				bCommitted = true;
			}
			break;
		}

		const bool bBagReorder =
			Ori == EDFItemDragOrigin::BagSlot ||
			(Ori == EDFItemDragOrigin::None && !P->bFromEquipmentSlot);
		if (bBagReorder && P->SourceInventorySlotIndex != INDEX_NONE)
		{
			if (P->SourceInventorySlotIndex == SlotIndex)
			{
				break;
			}
			if (UDFInventoryComponent* const Inv = PC->GetDFInventory())
			{
				if (!Inv->IsSlotValidIndex(P->SourceInventorySlotIndex) ||
					!Inv->IsSlotValidIndex(SlotIndex))
				{
					break;
				}
				Inv->RequestMoveBagSlot(P->SourceInventorySlotIndex, SlotIndex);
				bCommitted = true;
			}
			break;
		}
	} while (false);

	RefreshFromInventory();
	return bCommitted;
}

void UDFInventorySlotWidget::RequestEquipFromThisSlotUsingResolvedEquipmentSlot()
{
	ADFPlayerCharacter* const PC = GetDFPlayerCharacter();
	if (!PC)
	{
		return;
	}
	UDFInventoryComponent* const Inv = PC->GetDFInventory();
	UDFEquipmentComponent* const Eq  = PC->GetDFEquipment();
	if (!Inv || !Eq || !Inv->IsSlotValidIndex(SlotIndex))
	{
		return;
	}

	const FDFInventorySlot& S = Inv->Items[SlotIndex];
	if (S.RowName.IsNone() || S.Quantity < 1)
	{
		return;
	}

	const FDFItemTableRow* const Row = Inv->GetItemData(S.RowName);
	if (!Row)
	{
		return;
	}

	EEquipmentSlot Target = (Row->TargetEquipmentSlot != EEquipmentSlot::None)
		? Row->TargetEquipmentSlot
		: UDFEquipmentComponent::ResolveItemEquipmentSlot(*Row);

	if (Target == EEquipmentSlot::None)
	{
		return;
	}

	FString Reason;
	if (Row->ItemType == EItemType::Ring && Row->TargetEquipmentSlot == EEquipmentSlot::None)
	{
		// Try Ring1 first, fall back to Ring2.
		if (Eq->PredictCanEquipItem(S.RowName, EEquipmentSlot::Ring1, Reason, SlotIndex))
		{
			Target = EEquipmentSlot::Ring1;
		}
		else if (Eq->PredictCanEquipItem(S.RowName, EEquipmentSlot::Ring2, Reason, SlotIndex))
		{
			Target = EEquipmentSlot::Ring2;
		}
		else
		{
			return;
		}
	}
	else
	{
		if (!Eq->PredictCanEquipItem(S.RowName, Target, Reason, SlotIndex))
		{
			return;
		}
	}

	Eq->RequestEquipItem(S.RowName, Target, SlotIndex);
}