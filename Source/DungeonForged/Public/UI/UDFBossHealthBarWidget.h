// Source/DungeonForged/Public/UI/UDFBossHealthBarWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFBossHealthBarWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;
class ADFBossBase;
struct FGameplayTag;

UCLASS(Blueprintable, BlueprintType)
class DUNGEONFORGED_API UDFBossHealthBarWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	/** Binds to boss ASC + phase/enrage delegates; shows the bar. */
	UFUNCTION(BlueprintCallable, Category = "DF|Boss|UI")
	void ShowForBoss(ADFBossBase* Boss, const FText& DisplayName);

	UFUNCTION(BlueprintCallable, Category = "DF|Boss|UI")
	void HideBossBar();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void StartRebindTimer();
	void StopRebindTimer();
	void OnRebindTimerTick();
	void TryBindBossAttributes();
	void BindBossTagEvents();
	void UnbindBossTagEvents();
	void OnHealthAttrChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthAttrChanged(const FOnAttributeChangeData& Data);
	void OnPhaseChanged(int32 OldPhase, int32 NewPhase, AActor* Boss);
	void OnEnraged(AActor* Boss, bool bEnraged);
	void RefreshHealthFill();
	void RefreshEnrageCountdown();
	void RefreshVulnerableCallout(bool bVisible);
	void OnVulnerableTagChanged(FGameplayTag Tag, int32 NewCount);
	void ClearBossBindings();
	void StartHudRefreshTimer();
	void StopHudRefreshTimer();
	void OnHudRefreshTick();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BossHealthBar = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BossNameText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PhaseText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> EnrageIcon = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EnrageCountdownText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VulnerableCalloutText = nullptr;

	TWeakObjectPtr<ADFBossBase> TrackedBoss;
	bool bBossAttributesBound = false;
	FTimerHandle RebindTimerHandle;
	FTimerHandle HudRefreshTimerHandle;
	FDelegateHandle VulnerableTagDelegateHandle;
	static constexpr float RebindIntervalSec = 0.25f;
	static constexpr float HudRefreshIntervalSec = 0.2f;
};
