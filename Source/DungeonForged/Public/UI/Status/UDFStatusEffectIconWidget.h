// Source/DungeonForged/Public/UI/Status/UDFStatusEffectIconWidget.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Blueprint/UserWidget.h"
#include "UI/Status/DFStatusEffectData.h"
#include "UDFStatusEffectIconWidget.generated.h"

class UAbilitySystemComponent;
class UButton;
class UImage;
class UProgressBar;
class UTextBlock;
class UDFStatusEffectBarWidget;
class UDFEnemyDebuffStatusBarWidget;
class UDFStatusLibrary;
class UWidgetAnimation;

UCLASS(Abstract, Blueprintable)
class DUNGEONFORGED_API UDFStatusEffectIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeForPlayerBar(
		FGameplayTag InDisplayKey,
		UAbilitySystemComponent* InASC,
		FActiveGameplayEffectHandle InHandle,
		const FDFStatusEffectDisplayData& Data,
		UDFStatusEffectBarWidget* InHost,
		const UDFStatusLibrary* InLibrary);

	void InitializeForEnemyBar(
		FGameplayTag InDisplayKey,
		UAbilitySystemComponent* InASC,
		FActiveGameplayEffectHandle InHandle,
		const FDFStatusEffectDisplayData& Data,
		UDFEnemyDebuffStatusBarWidget* InHost,
		const UDFStatusLibrary* InLibrary);

	/** Size box / slot target; default 32 for player, 24 for enemy. */
	UFUNCTION(BlueprintCallable, Category = "DF|UI|Status")
	void SetDesiredIconSize(float SideLength);

	void StopAll();
	/** Return icon to the pool: clears handle/tag so it can be re-used. */
	void ResetForPool();
	void PlayFadeOutAndRequestReturn();

	FActiveGameplayEffectHandle GetEffectHandle() const { return ActiveHandle; }
	FGameplayTag GetDisplayKey() const { return DisplayKey; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;

	void TickDuration();
	void UpdateDurationDisplay(float Remaining, float Total);
	void ApplyDisplayData(const FDFStatusEffectDisplayData& Data);
	void ShowTooltip();
	void HideTooltip();

	UFUNCTION()
	void OnHoverAreaHovered();
	UFUNCTION()
	void OnHoverAreaUnhovered();

	void FinishFadeAndReturn();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> BorderImage = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DurationText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> DurationBar = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> HoverArea = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> FadeOutAnim = nullptr;

	FGameplayTag DisplayKey;
	TWeakObjectPtr<UAbilitySystemComponent> WatchedASC;
	FActiveGameplayEffectHandle ActiveHandle;
	FDFStatusEffectDisplayData CachedData;
	TWeakObjectPtr<const UDFStatusLibrary> DisplayLibrary;

	TWeakObjectPtr<UDFStatusEffectBarWidget> PlayerHost;
	TWeakObjectPtr<UDFEnemyDebuffStatusBarWidget> EnemyHost;

	FTimerHandle DurationTimer;
	TObjectPtr<class UDFStatusEffectTooltipWidget> ActiveTooltip = nullptr;

	float TotalDurationCached = 0.f;
	bool bUseDurationUi = true;
};
