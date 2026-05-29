// Source/DungeonForged/Public/UI/Combat/UDFDamageDirectionWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UDFDamageDirectionWidget.generated.h"

class UImage;

/** Screen-edge damage indicator — optional 360° radial arrow or 4-way cardinal fallback. */
UCLASS(Blueprintable)
class DUNGEONFORGED_API UDFDamageDirectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DF|UI|Damage")
	void PulseFromWorldLocation(const FVector& DamageSourceWorldLocation, float Intensity = 1.f);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Optional single arrow rotated toward damage source (360°). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Indicator_Radial = nullptr;

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

	UPROPERTY(EditAnywhere, Category = "DF|UI|Damage", meta = (ClampMin = "1", ClampMax = "6"))
	int32 MaxConcurrentPulses = 4;

private:
	struct FDamagePulseSlot
	{
		TWeakObjectPtr<UImage> Image;
		float Elapsed = 0.f;
		float TotalDuration = 0.f;
		float PeakOpacity = 0.f;
	};

	void StartPulseOnImage(UImage* Target, float Intensity);
	UImage* ResolveCardinalIndicator(float Dot, float Side) const;
	void StepPulseSlot(FDamagePulseSlot& Slot, float DeltaTime);
	void HideImage(UImage* Image) const;

	TArray<FDamagePulseSlot> ActivePulses;
};
