// Source/DungeonForged/Public/UI/UDFInGameHUDWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/UDFUserWidgetBase.h"
#include "UI/Status/UDFStatusEffectBarWidget.h"
#include "GameplayTagContainer.h"
#include "UDFInGameHUDWidget.generated.h"

class UTextBlock;
class UImage;
class UWidgetAnimation;
class UCanvasPanel;
class UDFAbilityHotbarWidget;
class UDFPlayerVitalsWidget;
class UDFDamageDirectionWidget;
struct FGameplayTag;

UCLASS(Blueprintable, Abstract)
class DUNGEONFORGED_API UDFInGameHUDWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void OnPlayerDamageTaken(FVector DamageSourceWorldLocation, float Intensity);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void HandleReplicatedRunGold(int32 NewTotal);

	void PlayGoldPulse();
	void BindCombatState();
	void UnbindCombatState();
	void OnInCombatTagChanged(const FGameplayTag Tag, int32 NewCount);
	void ApplyHUDOpacity(float Opacity);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GoldText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CoinIcon = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> GoldChangePulseAnim = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFStatusEffectBarWidget> DFStatusEffectBar = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFPlayerVitalsWidget> PlayerVitals = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFAbilityHotbarWidget> AbilityHotbar = nullptr;

	/** Optional parent for fadeable HUD cluster (vitals/hotbar/status). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> Panel_FadeableHUD = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFDamageDirectionWidget> DamageDirection = nullptr;

	UPROPERTY(EditAnywhere, Category = "DF|UI|HUD")
	float HUDFadeInDuration = 0.2f;

	UPROPERTY(EditAnywhere, Category = "DF|UI|HUD")
	float HUDFadeOutDuration = 1.2f;

	int32 LastGoldShown = 0;
	float CurrentHUDOpacity = 1.f;
	float TargetHUDOpacity = 1.f;
	FDelegateHandle CombatTagDelegateHandle;
};
