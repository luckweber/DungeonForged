// Source/DungeonForged/Public/Camera/UDFCameraComponent.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "UDFCameraComponent.generated.h"

class AActor;

UENUM(BlueprintType)
enum class ECameraState : uint8
{
	Default UMETA(DisplayName = "Default"),
	Combat  UMETA(DisplayName = "Combat"),
	LockOn  UMETA(DisplayName = "LockOn"),
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFCameraComponent : public USpringArmComponent
{
	GENERATED_BODY()

public:
	UDFCameraComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Smooth arm length + target offset. Call from TickComponent. */
	UFUNCTION(BlueprintCallable, Category = "DF|Camera")
	void TickCamera(float DeltaTime);

	/** Mouse / stick zoom: adjusts internal offset toward the next arm length, clamped. */
	UFUNCTION(BlueprintCallable, Category = "DF|Camera")
	void OnZoomInput(float AxisValue);

	UFUNCTION(BlueprintCallable, Category = "DF|Camera")
	void EnterCombatMode();

	UFUNCTION(BlueprintCallable, Category = "DF|Camera")
	void ExitCombatMode();

	/** Begin lock-on: switches socket offset, arm target, and enables rotation follow. */
	UFUNCTION(BlueprintCallable, Category = "DF|Camera")
	void EnableLockOn(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "DF|Camera")
	void DisableLockOn();

	/** Slerp control rotation to face the lock-on target (yaw + pitch from aim vector). */
	UFUNCTION(BlueprintCallable, Category = "DF|Camera")
	void UpdateLockOnRotation(float DeltaTime);

	/** Spring arm camera collision: probe + collision test. */
	UFUNCTION(BlueprintCallable, Category = "DF|Camera")
	void HandleCameraOcclusion();

	ECameraState GetCameraState() const { return CurrentState; }

	AActor* GetLockOnTarget() const { return LockOnTarget.Get(); }

protected:
	/** Spring arm / shoulder defaults */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Camera|Framing", meta = (ClampMin = "0.0"))
	float DefaultArmLength = 400.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Camera|Framing", meta = (ClampMin = "0.0"))
	float CombatArmLength = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Camera|Framing", meta = (ClampMin = "0.0"))
	float LockOnArmLength = 350.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Camera|Framing", meta = (ClampMin = "0.0"))
	float MinZoom = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Camera|Framing", meta = (ClampMin = "0.0"))
	float MaxZoom = 800.f;

	/** Scales OnZoomInput axis (character passes wheel * CameraZoomStep/ZoomSpeed to match old zoom feel). */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Camera|Framing", meta = (ClampMin = "0.0"))
	float ZoomSpeed = 50.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Camera|Framing", meta = (ClampMin = "0.0"))
	float InterpSpeed = 8.f;

	/** View offset at end of arm (over-the-shoulder). */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Camera|Framing")
	FVector DefaultSocketOffset = FVector(0.f, 60.f, 80.f);

	UPROPERTY(EditDefaultsOnly, Category = "DF|Camera|Framing")
	FVector LockOnSocketOffset = FVector(0.f, 80.f, 60.f);

	/** Drives default spring-arm camera lag. */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Camera|Lag", meta = (ClampMin = "0.0"))
	float CameraLag = 0.12f;

	/** How long enter/exit combat mode blends arm length (seconds). */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Camera|Combat", meta = (ClampMin = "0.01"))
	float CombatModeBlendDuration = 0.5f;

	/** When leaving lock-on, return to this state. */
	ECameraState StateBeforeLockOn = ECameraState::Default;

	ECameraState CurrentState = ECameraState::Default;

	/** Additional arm length on top of the state base, modified by OnZoom (clamped with Min/Max). */
	float UserArmOffset = 0.f;

	/** Lerpable goal for the spring arm (state base + user offset, clamped). */
	float StateGoalArmLength = 400.f;

	/** For smooth spring arm and socket offset. */
	FVector SmoothedSocketOffset;

	/** Blended state arm before user zoom, used by combat transition. */
	float BlendedStateArm = 400.f;

	bool bCombatBlendActive = false;
	bool bCombatBlendToCombat = true;
	float CombatBlendElapsed = 0.f;
	float CombatBlendFromArm = 400.f;
	float CombatBlendToArm = 300.f;

	TWeakObjectPtr<AActor> LockOnTarget;

	/** Clamps UserArmOffset so (BlendedStateArm + UserArmOffset) lies in [MinZoom,MaxZoom]. */
	void ClampUserArmOffset();
	float GetTargetSocketOffsetForState() const;
};
