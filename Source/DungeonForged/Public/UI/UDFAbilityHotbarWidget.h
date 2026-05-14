// Source/DungeonForged/Public/UI/UDFAbilityHotbarWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFAbilityHotbarWidget.generated.h"

class UDFAbilitySlotWidget;
class UDataTable;

UCLASS(Blueprintable, BlueprintType)
class DUNGEONFORGED_API UDFAbilityHotbarWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DF|UI|Hotbar")
	void RefreshHotbar();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UDataTable* ResolveAbilityDataTable() const;
	void CollectSlots();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilitySlotWidget> AbilitySlot1 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilitySlotWidget> AbilitySlot2 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilitySlotWidget> AbilitySlot3 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilitySlotWidget> AbilitySlot4 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilitySlotWidget> Slot1 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilitySlotWidget> Slot2 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilitySlotWidget> Slot3 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilitySlotWidget> Slot4 = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|UI|Hotbar")
	TArray<FText> InputLabels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|UI|Hotbar", meta = (ClampMin = "0.05"))
	float RefreshInterval = 0.25f;

	TArray<TObjectPtr<UDFAbilitySlotWidget>> Slots;
	TArray<FName> LastShownAbilityRows;
	float RefreshAccumulator = 0.f;
};
