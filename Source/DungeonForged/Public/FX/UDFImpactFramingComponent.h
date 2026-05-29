// Source/DungeonForged/Public/FX/UDFImpactFramingComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FX/UDFCombatFeedbackTypes.h"
#include "UDFImpactFramingComponent.generated.h"

class UAnimMontage;

/**
 * Per-actor montage rate-scale on confirmed hits (attacker swing or victim hit-react).
 * Independent from global hit-stop; supports multi-hit by extending the freeze window.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFImpactFramingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFImpactFramingComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|ImpactFraming", meta = (ClampMin = "0.0", ClampMax = "0.20"))
	float LightFreezeDuration = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|ImpactFraming", meta = (ClampMin = "0.0", ClampMax = "0.20"))
	float HeavyFreezeDuration = 0.07f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|ImpactFraming", meta = (ClampMin = "0.0", ClampMax = "0.20"))
	float CriticalFreezeDuration = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|ImpactFraming", meta = (ClampMin = "0.01", ClampMax = "0.5"))
	float FreezeRate = 0.05f;

	UFUNCTION(BlueprintCallable, Category = "DF|ImpactFraming")
	void TriggerLight() { TriggerCustom(LightFreezeDuration); }

	UFUNCTION(BlueprintCallable, Category = "DF|ImpactFraming")
	void TriggerHeavy() { TriggerCustom(HeavyFreezeDuration); }

	UFUNCTION(BlueprintCallable, Category = "DF|ImpactFraming")
	void TriggerCritical() { TriggerCustom(CriticalFreezeDuration); }

	UFUNCTION(BlueprintCallable, Category = "DF|ImpactFraming")
	void TriggerCustom(float Duration);

	/** Victim/attacker montage rate-scale for a combat feedback band (client-side). */
	UFUNCTION(BlueprintCallable, Category = "DF|ImpactFraming")
	void PlayBand(EDFHitFeedbackBand Band, float MagnitudeFactor = 1.f);

	/** Wall-clock seconds left in the active montage freeze (screen FX sync). */
	UFUNCTION(BlueprintPure, Category = "DF|ImpactFraming")
	float GetActiveFreezeRemainingSeconds() const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void RestoreRate();
	class UAnimInstance* GetAnimInstance() const;

	void ApplyFreezeToMontage(UAnimMontage* Montage, float Duration, float Rate);

	FTimerHandle RestoreTimer;
	TWeakObjectPtr<UAnimMontage> FrozenMontage;
	float PriorRate = 1.f;
	float ActiveFreezeRate = 1.f;
	double FreezeEndRealTime = 0.0;
	bool bRateActive = false;
};
