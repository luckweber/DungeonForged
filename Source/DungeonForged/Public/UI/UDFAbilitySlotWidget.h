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

	/**
	 * Tag used with FGameplayEffectQuery::MakeQuery_MatchAnyEffectTags.
	 * Add this tag to your cooldown GameplayEffect's effect tags (or use asset tags and adjust the query in cpp to MatchAnyOwningTags).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|UI|GAS", meta = (Categories = "Ability.Cooldown"))
	FGameplayTag AbilityTag;

	/** 0 = ready, 1 = full remaining cooldown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|UI|VFX")
	FName CooldownMaterialParameter = TEXT("CooldownPercent");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|UI|VFX")
	bool bCreateDynamicMaterialInConstruct = true;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void HandleActiveGameplayEffectAdded(
		UAbilitySystemComponent* Target, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);

	void OnCooldownUpdateTimer();
	void UpdateCooldownVisuals();
	void ClearCooldownUI();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> AbilityIcon = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CooldownOverlay = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CooldownText = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|UI|Art")
	TObjectPtr<UTexture2D> AbilityIconTexture = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CooldownOverlayMID = nullptr;

	TWeakObjectPtr<UAbilitySystemComponent> CooldownSourceASC;
	FDelegateHandle OnActiveGEAddedHandle;

	FTimerHandle CooldownUpdateTimerHandle;
};
