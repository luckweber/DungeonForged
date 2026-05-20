// Source/DungeonForged/Public/FX/UDFHitStopSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "FX/UDFCombatFeedbackTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "UDFHitStopSubsystem.generated.h"

class AActor;

/**
 * Global time dilation "hit stop" with optional actor exclusion (CustomTimeDilation = 1/EffectiveGlobal).
 * End time uses FPlatformTime (real time) so duration is wall-clock and not affected by the dilation itself.
 */
UCLASS()
class DUNGEONFORGED_API UDFHitStopSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;
	virtual void Deinitialize() override;

	/** Global time dilation "hit stop" with optional actor exclusion (CustomTimeDilation = 1/EffectiveGlobal). */
	UFUNCTION(BlueprintPure, Category = "DF|FX|HitStop")
	bool IsHitStopActive() const { return bInHitStop; }

	/** Alias used by combo buffer pause (C2). */
	UFUNCTION(BlueprintPure, Category = "DF|FX|HitStop")
	bool IsCurrentlyDilated() const { return bInHitStop; }

	/** Wall-clock seconds remaining in the active hit-stop window (F9 sync). */
	UFUNCTION(BlueprintPure, Category = "DF|FX|HitStop")
	float GetHitStopRemainingSeconds() const;

	/** Scales duration within the band using @a MagnitudeFactor (0.5–1.5 typical). */
	UFUNCTION(BlueprintCallable, Category = "DF|FX|HitStop")
	void PlayBand(EDFHitFeedbackBand Band, AActor* ExcludeActor = nullptr, float MagnitudeFactor = 1.f);

	/** @param TimeDilation Global time dilation; values near 0 are clamped for engine/tickability. */
	UFUNCTION(BlueprintCallable, Category = "DF|FX|HitStop")
	void TriggerHitStop(float Duration, float TimeDilation, AActor* ExcludeActor = nullptr);

	UFUNCTION(BlueprintCallable, Category = "DF|FX|HitStop")
	void LightHit(AActor* ExcludeActor = nullptr) { TriggerHitStop(0.06f, 0.05f, ExcludeActor); }

	UFUNCTION(BlueprintCallable, Category = "DF|FX|HitStop")
	void HeavyHit(AActor* ExcludeActor = nullptr) { TriggerHitStop(0.10f, 0.02f, ExcludeActor); }

	UFUNCTION(BlueprintCallable, Category = "DF|FX|HitStop")
	void CriticalHit(AActor* ExcludeActor = nullptr) { TriggerHitStop(0.14f, 0.01f, ExcludeActor); }

	/** Near-freeze: uses minimum positive global dilation so the world (and this subsystem) can still tick. */
	UFUNCTION(BlueprintCallable, Category = "DF|FX|HitStop")
	void BossSlam(AActor* ExcludeActor = nullptr) { TriggerHitStop(0.20f, 0.0f, ExcludeActor); }

	/** Clears an active hit-stop window (e.g. when skipping defeat UI). */
	UFUNCTION(BlueprintCallable, Category = "DF|FX|HitStop")
	void ForceEndHitStop();

protected:
	/** If BossSlam requests 0.0 global dilation, use this so ticks and real-time end still work. */
	static float SafeGlobalDilation(float Requested);

	void ApplyHitStop(float TimeDilation, AActor* ExcludeActor);
	void EndHitStop();
	void SetExcludedActorDilation(AActor* ExcludeActor, float GlobalDilation);

	bool bInHitStop = false;
	double HitStopEndRealTime = 0.0;
	TWeakObjectPtr<AActor> ExcludedActor;

	static constexpr float MinGlobalDilation = 0.0001f;
};
