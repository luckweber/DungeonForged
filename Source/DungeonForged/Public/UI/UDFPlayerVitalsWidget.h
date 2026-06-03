// Source/DungeonForged/Public/UI/UDFPlayerVitalsWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFPlayerVitalsWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS(Blueprintable, BlueprintType)
class DUNGEONFORGED_API UDFPlayerVitalsWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DF|UI|Vitals")
	void RefreshVitals();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void TryBindVitals();
	void OnAnyVitalAttributeChanged(const FOnAttributeChangeData& Data);
	void SetResourceWidgets(UProgressBar* Bar, UTextBlock* Text, float Current, float MaxValue, const FText& Label) const;
	void UpdateHealthLagBar(float ActualPercent);
	void UpdateLowHealthBarPulse(float ActualPercent, float DeltaTime);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar = nullptr;

	/** Optional trailing bar shown behind @c HealthBar when taking damage. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthLagBar = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ManaBar = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> StaminaBar = nullptr;

	/** Optional aliases for globe-style materials/layouts. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthOrb = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ManaOrb = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ManaText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StaminaText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthPercentText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ManaPercentText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StaminaPercentText = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|UI|Vitals")
	bool bShowLabelsInValueText = false;

	/** Trailing health bar catch-up speed (percent/sec). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|UI|Vitals", meta = (ClampMin = "0.5"))
	float HealthLagDecaySpeed = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|UI|Vitals", meta = (ClampMin = "0.05", ClampMax = "0.5"))
	float LowHealthThreshold = 0.25f;

	float RebindAccumulator = 0.f;
	bool bVitalsBound = false;
	float DisplayedLagHealthPercent = 1.f;
	float TargetHealthPercent = 1.f;
	float LowHealthPulsePhase = 0.f;
	FLinearColor DefaultHealthFillColor = FLinearColor(0.85f, 0.12f, 0.08f, 1.f);
};
