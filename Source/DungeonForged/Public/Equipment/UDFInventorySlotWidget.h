// Source/DungeonForged/Public/Equipment/UDFInventorySlotWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Input/Events.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFInventorySlotWidget.generated.h"

class APlayerController;
class APawn;
class UDragDropOperation;
class UImage;
class UTextBlock;
class UDFInventoryComponent;

/**
 * One bag slot: refresh from UDFInventoryComponent, drag-to-equip, accept equipment drags (unequip → bag).
 * Set a unique SlotIndex per widget in Designer (0..MaxSlots-1), or SetBagSlotIndex from Blueprint/runtime.
 */
UCLASS(Abstract, Blueprintable)
class DUNGEONFORGED_API UDFInventorySlotWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	/** Assign at runtime/layout time; assigning SlotIndex in Blueprint graphs is intentionally read-only. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "DF|Inventory|UI",
		meta = (ClampMin = "-1",
			UIMin = "-1",
			ToolTip = "Bag index [0 .. MaxSlots-1], unique per slot widget; -1 clears display until configured."))
	int32 SlotIndex = INDEX_NONE;

	UFUNCTION(BlueprintCallable, Category = "DF|Inventory|UI")
	void SetBagSlotIndex(int32 InIndex);

	UFUNCTION(BlueprintCallable, Category = "DF|Inventory|UI")
	void RefreshFromInventory();

	/** Resolves target equipment slot from DT row (rings try Ring1 then Ring2). */
	UFUNCTION(BlueprintCallable, Category = "DF|Inventory|UI")
	void RequestEquipFromThisSlotUsingResolvedEquipmentSlot();

	/** Multiplier over the SlotIcon geometry size when building drag preview visuals. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Inventory|UI", meta = (ClampMin = "1.0"))
	float DragPreviewIconScale = 1.45f;

	/** Clamp after scale so previews never feel tiny next to coarse slots. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Inventory|UI")
	FVector2D DragPreviewMinimumSize = FVector2D(56.f, 56.f);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent,
		UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;
	virtual void NativeOnDragEnter(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
		override;
	virtual bool NativeOnDragOver(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SlotIcon = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> RarityBorder = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuantityText = nullptr;

private:
	void UpdateBagSlotDragHoverVisualFromOperation(UDragDropOperation* InOperation);

	bool HasStackForDrag() const;

	void RegisterOwningPlayerPossessListener();
	void UnregisterOwningPlayerPossessListener();
	void BindInventoryChangedDelegate();
	void UnbindInventoryChangedDelegate();

	UFUNCTION()
	void HandleOwningPlayerPossessedChanged(APawn* PreviousPawn, APawn* NewPawn);

	UFUNCTION()
	void HandleInventoryChangedBroadcast();

	TWeakObjectPtr<APlayerController> OwningPossessPcWeak;
	TWeakObjectPtr<UDFInventoryComponent> BoundInventoryWeak;
};
