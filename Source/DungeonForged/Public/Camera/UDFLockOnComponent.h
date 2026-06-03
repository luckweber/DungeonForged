// Source/DungeonForged/Public/Camera/UDFLockOnComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDFLockOnComponent.generated.h"

class AActor;
class UDFCameraComponent;
class UDFLockOnWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnUDFLockOnChanged, bool /* bIsLockedOn */);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFLockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFLockOnComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Sphere + cone + LOS: highest-scored valid enemy, starts camera lock. */
	UFUNCTION(BlueprintCallable, Category = "DF|LockOn")
	bool TryLockOn();

	/** Direction +1 = next, -1 = previous (e.g. Q / E or stick flick). */
	UFUNCTION(BlueprintCallable, Category = "DF|LockOn")
	void CycleLockOnTarget(float Direction);

	UFUNCTION(BlueprintCallable, Category = "DF|LockOn")
	void ReleaseLockOn();

	/** Switch lock to a confirmed hit victim (local player, when enabled). */
	UFUNCTION(BlueprintCallable, Category = "DF|LockOn")
	void NotifyCombatHitConfirmed(AActor* HitVictim);

	/** Best scored candidate in view (soft aim when not hard locked). */
	UFUNCTION(BlueprintPure, Category = "DF|LockOn")
	AActor* GetSoftTarget() const;

	/** World-space indicator follow (call every frame while locked, local only). */
	UFUNCTION(BlueprintCallable, Category = "DF|LockOn")
	void UpdateIndicator(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "DF|LockOn")
	bool IsTargetValid(AActor* Target) const;

	UFUNCTION(BlueprintPure, Category = "DF|LockOn")
	bool IsLockedOn() const { return bIsLockedOn; }

	UFUNCTION(BlueprintPure, Category = "DF|LockOn")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	UFUNCTION(BlueprintPure, Category = "DF|LockOn")
	float GetLockOnRange() const { return LockOnRange; }

	UFUNCTION(BlueprintPure, Category = "DF|LockOn")
	float GetLockOnAngle() const { return LockOnAngle; }

	/** Fired when lock-on is acquired (true) or released (false). */
	FOnUDFLockOnChanged OnLockOnChanged;

protected:
	TWeakObjectPtr<AActor> CurrentTarget;
	bool bIsLockedOn = false;

	/** Autodetect: spring arm on owner with same class. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|LockOn")
	TObjectPtr<UDFCameraComponent> Camera;

	UPROPERTY(EditDefaultsOnly, Category = "DF|LockOn", meta = (ClampMin = "0.0"))
	float LockOnRange = 1500.f;

	/** Full cone angle in front of the view (degrees). */
	UPROPERTY(EditDefaultsOnly, Category = "DF|LockOn", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float LockOnAngle = 60.f;

	/** Grace period before auto-break when target leaves range/LOS (seconds). */
	UPROPERTY(EditDefaultsOnly, Category = "DF|LockOn", meta = (ClampMin = "0.0"))
	float AutoBreakGraceDelay = 0.4f;

	float TimeTargetInvalid = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|LockOn|Scoring", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ScoreCameraWeight = 0.40f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|LockOn|Scoring", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ScoreDistanceWeight = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|LockOn|Scoring", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ScoreThreatWeight = 0.20f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|LockOn|Scoring", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ScoreElevationWeight = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|LockOn|Scoring", meta = (ClampMin = "50.0"))
	float ElevationTolerance = 400.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|LockOn")
	bool bRetargetOnHit = true;

	UPROPERTY(EditDefaultsOnly, Category = "DF|LockOn")
	bool bSoftAimWhenUnlocked = true;

	/** If true, a widget instance was created for this local player. */
	bool bWidgetCreated = false;

	/** If set, only this class and subclasses are valid lock targets (e.g. ADFEnemyBase). */
	UPROPERTY(EditDefaultsOnly, Category = "DF|LockOn")
	TSubclassOf<AActor> LockTargetClass;

	UPROPERTY(EditDefaultsOnly, Category = "DF|LockOn|UI")
	TSubclassOf<UDFLockOnWidget> LockOnWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|LockOn|UI")
	TObjectPtr<UDFLockOnWidget> LockOnWidget;

	int32 LockCycleIndex = 0;

	/** Reordered each time cycle is used; stores weak refs to in-range candidates. */
	TArray<TWeakObjectPtr<AActor>> CandidateBuffer;

	enum class ELockOnCandidateSort : uint8
	{
		Score,
		ViewAngle,
	};

	bool BuildCandidates(TArray<AActor*>& OutSorted, ELockOnCandidateSort SortMode) const;
	float ScoreTarget(AActor* Target) const;
	float GetThreatScore(AActor* Target) const;
	void GetViewPoint(FVector& OutOrigin, FVector& OutForward) const;
	float AngleFromView(AActor* Target) const;
	float SignedViewAngle(AActor* Target) const;
	bool IsActorValidEnemyType(AActor* Actor) const;
	/** Acquire / cycle: range + view cone + LOS + alive. */
	bool IsTargetValidForAcquire(AActor* Target) const;
	/** While locked: range + LOS + alive only (no cone — dodge/roll can leave the frontal arc). */
	bool IsTargetValidForMaintain(AActor* Target) const;
	bool IsOwnerDodging() const;
	bool HasLineOfSight(AActor* Target) const;
	void SetCurrentTarget(AActor* NewTarget);
	void EnsureLockOnWidget();
};
