// Source/DungeonForged/Public/Equipment/UDFEquipmentSlotWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Equipment/DFEquipmentTypes.h"
#include "Input/Events.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFEquipmentSlotWidget.generated.h"

class APlayerController;
class APawn;
class UDragDropOperation;
class UImage;
class UTextBlock;
class UDFEquipmentComponent;
struct FDFItemTableRow;

UCLASS(Abstract, Blueprintable)
class DUNGEONFORGED_API UDFEquipmentSlotWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	/** Edit per-slot in Designer; Blueprint graphs must use SetEquipmentSlot. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "DF|Equipment|UI")
	EEquipmentSlot EquipSlot = EEquipmentSlot::None;

	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|UI")
	void SetEquipmentSlot(EEquipmentSlot InSlot);

	/** Pulls icon / level / rarity from the owning player's UDFEquipmentComponent. */
	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|UI")
	void RefreshFromEquipment();

	/** Drag/drop or context: try to equip from a specific bag grid index when known (-1 = any stack). */
	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|UI", meta = (AdvancedDisplay = "SourceBagSlotIndex"))
	bool RequestEquipItemRow(FName ItemRowName, int32 SourceBagSlotIndex = -1);

	/** Multiplier over SlotIcon geometry for drag previews. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Equipment|UI", meta = (ClampMin = "1.0"))
	float DragPreviewIconScale = 1.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Equipment|UI")
	FVector2D DragPreviewMinimumSize = FVector2D(56.f, 56.f);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
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

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemLevelText = nullptr;

private:
	void UpdateEquipSlotDragHoverVisualFromOperation(UDragDropOperation* InOperation);

	void RegisterOwningPlayerPossessListener();
	void UnregisterOwningPlayerPossessListener();
	void BindEquipmentChangedDelegate();
	void UnbindEquipmentChangedDelegate();

	UFUNCTION()
	void HandleOwningPlayerPossessedChanged(APawn* PreviousPawn, APawn* NewPawn);

	UFUNCTION()
	void HandleEquipmentChangedBroadcast(EEquipmentSlot ChangedEquipSlot, FName ItemRow);

	TWeakObjectPtr<APlayerController> OwningPossessPcWeak;
	TWeakObjectPtr<UDFEquipmentComponent> BoundEquipmentWeak;
};
