// Source/DungeonForged/Public/UI/Combat/UDFDamageDirectionWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UDFDamageDirectionWidget.generated.h"

class UImage;

/** Screen-edge damage indicator (4 optional edge images). */
UCLASS(Blueprintable)
class DUNGEONFORGED_API UDFDamageDirectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DF|UI|Damage")
	void PulseFromWorldLocation(const FVector& DamageSourceWorldLocation, float Intensity = 1.f);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Indicator_Top = nullptr;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Indicator_Right = nullptr;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Indicator_Bottom = nullptr;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Indicator_Left = nullptr;

	UPROPERTY(EditAnywhere, Category = "DF|UI|Damage")
	float PulseFadeInDuration = 0.1f;

	UPROPERTY(EditAnywhere, Category = "DF|UI|Damage")
	float PulseFadeOutDuration = 1.1f;

private:
	void PulseIndicator(UImage* Target, float Intensity);
	void ClearPulse();

	TWeakObjectPtr<UImage> ActiveIndicator;
	float PulseElapsed = 0.f;
	float PulseTotalDuration = 0.f;
	float PulsePeakOpacity = 0.f;
};
