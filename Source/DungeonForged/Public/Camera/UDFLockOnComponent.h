// Source/DungeonForged/Public/Camera/UDFLockOnComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDFLockOnComponent.generated.h"

class AActor;
class UDFCameraComponent;
class UDFLockOnWidget;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFLockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFLockOnComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Sphere + cone + LOS: nearest valid enemy, starts camera lock. */
	UFUNCTION(BlueprintCallable, Category = "DF|LockOn")
	bool TryLockOn();

	/** Direction +1 = next, -1 = previous (e.g. Q / E or stick flick). */
	UFUNCTION(BlueprintCallable, Category = "DF|LockOn")
	void CycleLockOnTarget(float Direction);

	UFUNCTION(BlueprintCallable, Category = "DF|LockOn")
	void ReleaseLockOn();

	/** World-space indicator follow (call every frame while locked, local only). */
	UFUNCTION(BlueprintCallable, Category = "DF|LockOn")
	void UpdateIndicator(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "DF|LockOn")
	bool IsTargetValid(AActor* Target) const;

	UFUNCTION(BlueprintPure, Category = "DF|LockOn")
	bool IsLockedOn() const { return bIsLockedOn; }

	UFUNCTION(BlueprintPure, Category = "DF|LockOn")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

protected:
	TWeakObjectPtr<AActor> CurrentTarget;
	bool bIsLockedOn = false;

	/** Autodetect: spring arm on owner with same class. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|LockOn")
	TObjectPtr<UDFCameraComponent> Camera;

	UPROPERTY(EditDefaultsOnly, Category = "DF|LockOn", meta = (ClampMin = "0.0"))
	float LockOnRange = 1500.f;

	/** Full cone angle in front of the player (degrees). */
	UPROPERTY(EditDefaultsOnly, Category = "DF|LockOn", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float LockOnAngle = 60.f;

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

	/** If true, a widget instance was created for this local player. */
	bool bWidgetCreated = false;

	bool BuildCandidatesInView(TArray<AActor*>& OutSorted) const;
	bool IsActorValidEnemyType(AActor* Actor) const;
	float AngleFromForward(AActor* Target) const;
	bool HasLineOfSight(AActor* Target) const;
	void EnsureLockOnWidget();
};
