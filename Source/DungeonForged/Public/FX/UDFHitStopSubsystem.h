// Source/DungeonForged/Public/FX/UDFHitStopSubsystem.h
#pragma once

#include "CoreMinimal.h"
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
