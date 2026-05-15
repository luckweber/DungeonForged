// Source/DungeonForged/Public/UI/UDFAbilitySlotWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameplayTagContainer.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFAbilitySlotWidget.generated.h"

class UImage;
class UTextBlock;
class UMaterialInstanceDynamic;
class UTexture2D;
struct FGameplayEffectSpec;

UCLASS(Blueprintable, BlueprintType)
class DUNGEONFORGED_API UDFAbilitySlotWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	UDFAbilitySlotWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "DF|UI|AbilitySlot")
	void SetAbilitySlotData(FGameplayTag InAbilityTag, UTexture2D* InIcon, FText InDisplayName, FText InInputLabel);

	UFUNCTION(BlueprintCallable, Category = "DF|UI|AbilitySlot")
	void ClearAbilitySlotData();

	UFUNCTION(BlueprintCallable, Category = "DF|UI|AbilityBar")
	void SetBarSlotIndex(int32 InSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "DF|UI|AbilityBar")
	void SetOwningHotbar(class UDFAbilityHotbarWidget* InHotbar);

	/**
	 * Tag of the ability on this slot (e.g. Ability.Warrior.MeleeSwing). Cooldown UI matches active GEs whose
	 * InheritableGameplayEffectTags include Ability.Cooldown plus this tag (see UGE_Cooldown_Base::CooldownAssociatedAbilityTag).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|UI|GAS", meta = (Categories = "Ability"))
	FGameplayTag AbilityTag;

	/** 0 = ready, 1 = full remaining cooldown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|UI|VFX")
	FName CooldownMaterialParameter = TEXT("CooldownPercent");

	/** Extra scalar on M_CoolDown (default `percent` — common on M_CoolDown_Inst). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|UI|VFX")
	FName CooldownAuxScalarParameter = TEXT("percent");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|UI|VFX")
	bool bCreateDynamicMaterialInConstruct = true;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	void HandleActiveGameplayEffectAdded(
		UAbilitySystemComponent* Target, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);

	void EnsureCooldownOverlayMID();
	void ApplyCooldownMaterialScalars(UMaterialInstanceDynamic* MID, float Pct) const;
	void TryBindCooldownDelegates(UAbilitySystemComponent* ASC);
	void UnbindCooldownDelegatesIfAny();

	void OnCooldownUpdateTimer();
	void UpdateCooldownVisuals();
	void ClearCooldownUI();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> AbilityIcon = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CooldownOverlay = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CooldownText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AbilityNameText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InputLabelText = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|UI|Art")
	TObjectPtr<UTexture2D> AbilityIconTexture = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CooldownOverlayMID = nullptr;

	TWeakObjectPtr<UAbilitySystemComponent> CooldownSourceASC;
	FDelegateHandle OnActiveGEAddedHandle;

	FTimerHandle CooldownUpdateTimerHandle;

	int32 BarSlotIndex = INDEX_NONE;
	TWeakObjectPtr<class UDFAbilityHotbarWidget> OwningHotbar;
};
