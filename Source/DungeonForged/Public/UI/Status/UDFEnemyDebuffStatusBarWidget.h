// Source/DungeonForged/Public/UI/Status/UDFEnemyDebuffStatusBarWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "UDFEnemyDebuffStatusBarWidget.generated.h"

class UAbilitySystemComponent;
class ADFEnemyBase;
class UDFStatusEffectIconWidget;
class UDFStatusLibrary;
class UHorizontalBox;
class UWidgetAnimation;
class UDFStatusEffectTooltipWidget;

/**
 * WBP above enemy health: up to 3 player-applied debuff icons.
 */
UCLASS(Abstract, Blueprintable)
class DUNGEONFORGED_API UDFEnemyDebuffStatusBarWidget : public UUserWidget
{
	GENERATED_BODY()

	friend class UDFStatusEffectIconWidget;

public:
	void SetupObservedEnemy(ADFEnemyBase* InEnemy, UDFStatusLibrary* InLibrary, UAbilitySystemComponent* InLocalFilterAsc);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|UI|Status")
	TSubclassOf<UDFStatusEffectIconWidget> StatusIconWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|UI|Status|Tooltip")
	TSubclassOf<UDFStatusEffectTooltipWidget> TooltipWidgetClass;

	void ReleaseIconToPool(UDFStatusEffectIconWidget* Icon);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void BindToEnemyAsc();
	void Unbind();
	void OnEffectAdded(UAbilitySystemComponent* Target, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);
	void OnEffectRemoved(const FActiveGameplayEffect& Removed);

	void RefreshActive();
	void LayoutTopThree();
	bool IsFromLocalPlayer(const FGameplayEffectSpec& Spec) const;

	TWeakObjectPtr<ADFEnemyBase> ObservedEnemy;
	TWeakObjectPtr<UAbilitySystemComponent> EnemyAsc;
	TWeakObjectPtr<UAbilitySystemComponent> LocalFilterAsc;
	TObjectPtr<UDFStatusLibrary> StatusLibrary = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> DebuffRow = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> IconAddAnim = nullptr;

	FDelegateHandle AddedHandle;
	FDelegateHandle RemovedHandle;

	TMap<FActiveGameplayEffectHandle, TObjectPtr<UDFStatusEffectIconWidget>> IconByHandle;
	TArray<TObjectPtr<UDFStatusEffectIconWidget>> Pooled;
};
