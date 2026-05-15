// Source/DungeonForged/Public/UI/UDFAbilityHotbarWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/DFAbilityBarTypes.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFAbilityHotbarWidget.generated.h"

class ADFPlayerCharacter;
class UDFAbilitySlotWidget;
class UDataTable;
class UProgressBar;

UCLASS(Blueprintable, BlueprintType)
class DUNGEONFORGED_API UDFAbilityHotbarWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DF|UI|Hotbar")
	void RefreshHotbar();

	/** Server swap via owning player (drag-and-drop between slots). */
	UFUNCTION(BlueprintCallable, Category = "DF|UI|AbilityBar")
	void RequestSwapSlots(int32 SlotIndexA, int32 SlotIndexB);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UDataTable* ResolveAbilityDataTable() const;
	void CollectSlots();
	void RefreshEmbeddedVitals() const;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthOrb = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ManaOrb = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> StaminaBar = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilitySlotWidget> AbilitySlot1 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilitySlotWidget> AbilitySlot2 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilitySlotWidget> AbilitySlot3 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilitySlotWidget> AbilitySlot4 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilitySlotWidget> AbilitySlot5 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilitySlotWidget> AbilitySlot6 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilitySlotWidget> AbilitySlot7 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilitySlotWidget> AbilitySlot8 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilitySlotWidget> AbilitySlot9 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilitySlotWidget> AbilitySlot10 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilitySlotWidget> AbilitySlot11 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilitySlotWidget> AbilitySlot12 = nullptr;

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

	void BindToPlayerCharacter();
	void UnbindFromPlayerCharacter();

	UFUNCTION()
	void HandleAbilityBarSlotsChanged();

	TWeakObjectPtr<ADFPlayerCharacter> BoundPlayerCharacter;
};
