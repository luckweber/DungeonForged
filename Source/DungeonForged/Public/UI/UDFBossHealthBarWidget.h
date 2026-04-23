// Source/DungeonForged/Public/UI/UDFBossHealthBarWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFBossHealthBarWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;
class ADFBossBase;

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
	virtual void NativeDestruct() override;

	void OnHealthAttrChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthAttrChanged(const FOnAttributeChangeData& Data);
	void OnPhaseChanged(int32 OldPhase, int32 NewPhase, AActor* Boss);
	void OnEnraged(AActor* Boss, bool bEnraged);
	void RefreshHealthFill();
	void ClearBossBindings();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BossHealthBar = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BossNameText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PhaseText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> EnrageIcon = nullptr;

	TWeakObjectPtr<ADFBossBase> TrackedBoss;
};
