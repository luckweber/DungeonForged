// Source/DungeonForged/Public/UI/UDFItemTooltipWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Data/DFDataTableStructs.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFItemTooltipWidget.generated.h"

class UDataTable;
class UTextBlock;
class UImage;

UCLASS(Blueprintable, Abstract)
class DUNGEONFORGED_API UDFItemTooltipWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	/** If bUseEquippedCompare, shows CompareText vs OptionalEquipped (same item type). */
	UFUNCTION(BlueprintCallable, Category = "DF|UI|Item")
	void SetItemData(
		const FDFItemTableRow& Row,
		const FDFItemTableRow& OptionalEquipped,
		bool bUseEquippedCompare,
		UDataTable* ItemTableForLookup);

	/**
	 * Per-attribute delta vs the item currently in the same equipment slot.
	 * When bEquippedSlotIsEmpty, StatBlock/CompareText highlight \"NEW\" for that slot.
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|UI|Item")
	void SetItemDataEx(
		const FDFItemTableRow& Hovered,
		const FDFItemTableRow& EquippedInSlot,
		bool bCompare,
		bool bEquippedSlotIsEmpty,
		UDataTable* ItemTableForLookup);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemName = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemType = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Description = nullptr;

	/** Newline-joined stat lines, or use Blueprint to replace with a vertical list. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatBlock = nullptr;

	/** e.g. \"▲ +10 vs equipped\" in green, \"▼ -5\" in red. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CompareText = nullptr;

	/** Optional icon for future use. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemIcon = nullptr;
};
