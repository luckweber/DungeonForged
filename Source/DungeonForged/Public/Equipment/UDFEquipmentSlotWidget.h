// Source/DungeonForged/Public/Equipment/UDFEquipmentSlotWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Equipment/DFEquipmentTypes.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFEquipmentSlotWidget.generated.h"

class UImage;
class UTextBlock;
struct FDFItemTableRow;

UCLASS(Abstract, Blueprintable)
class DUNGEONFORGED_API UDFEquipmentSlotWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	/** UWidget already has "Slot" — this is the equipment slot. */
	UPROPERTY(BlueprintReadOnly, Category = "DF|Equipment|UI")
	EEquipmentSlot EquipSlot = EEquipmentSlot::None;

	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|UI")
	void SetEquipmentSlot(EEquipmentSlot InSlot) { EquipSlot = InSlot; }

	/** Pulls icon / level / rarity from the owning player's UDFEquipmentComponent. */
	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|UI")
	void RefreshFromEquipment();

	/** Drag/drop or context: try to equip this row into this slot (server RPC via component). */
	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|UI")
	bool RequestEquipItemRow(FName ItemRowName);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SlotIcon = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> RarityBorder = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemLevelText = nullptr;
};
