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

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar = nullptr;

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

	float RebindAccumulator = 0.f;
	bool bVitalsBound = false;
};
