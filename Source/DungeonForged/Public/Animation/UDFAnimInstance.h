// Source/DungeonForged/Public/Animation/UDFAnimInstance.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Combat/DFDodgeTypes.h"
#include "Animation/DFAnimSetTypes.h"
#include "Animation/UDFLocomotionTypes.h"
#include "UDFAnimInstance.generated.h"

class ACharacter;
class UAbilitySystemComponent;
class UDFCharacterMovementComponent;
struct FGameplayTag;


UCLASS(Blueprintable)
class DUNGEONFORGED_API UUDFAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "DF|Anim Set")
	void ApplyAnimSet(const FUDAnimSet& NewAnimSet);

	UFUNCTION(BlueprintCallable, Category = "DF|Anim Set")
	void RevertToDefaultAnimSet();

	/** Root-motion montage: stash current movement and enter MOVE_Custom (for PhysCustom in project CMC). */
	UFUNCTION(BlueprintCallable, Category = "DF|RootMotion")
	void PushAnimNotifiedCustomMovement();

	/** Restore stashed mode after UDFAnimNotify_DisableRootMotion. */
	UFUNCTION(BlueprintCallable, Category = "DF|RootMotion")
	void PopAnimNotifiedCustomMovement();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DF|Locomotion")
	bool HasTag(const FGameplayTag& Tag) const;

	UFUNCTION(BlueprintPure, Category = "DF|Locomotion|Jump")
	UAnimSequenceBase* GetJumpStartAnim() const;

	UFUNCTION(BlueprintPure, Category = "DF|Locomotion|Jump")
	UAnimSequenceBase* GetJumpLoopAnim() const;

	UFUNCTION(BlueprintPure, Category = "DF|Locomotion|Jump")
	UAnimSequenceBase* GetJumpLandAnim() const;

	UFUNCTION(BlueprintPure, Category = "DF|Locomotion|Jump")
	UAnimSequenceBase* GetJumpDoubleStartAnim() const;

	UFUNCTION(BlueprintPure, Category = "DF|Locomotion|Jump")
	UAnimSequenceBase* GetJumpDoubleLoopAnim() const;

	/** Alpha 0..1 for early land blend: 1 when close to ground while falling. */
	UFUNCTION(BlueprintPure, Category = "DF|Locomotion|Jump")
	float GetLandPreparationAlpha() const;

	/** Mirrors recommended Main States SM rules — wire transitions to these for tuning. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump|Transitions")
	bool bTransition_LocomotionToJumpStart = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump|Transitions")
	bool bTransition_JumpStartToLoop = false;

	/** Locomotion → Jump Loop when past start window (air dash resume / mid-fall re-entry). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump|Transitions")
	bool bTransition_LocomotionToJumpLoop = false;

	/** Jump Loop → Locomotion when grounded (wire instead of inverting KeepLoop in AnimBP). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump|Transitions")
	bool bTransition_JumpLoopToLocomotion = false;

	/** Block Jump Loop / Jump Start → Locomotion while still airborne (wire as NOT on escape transitions). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump|Transitions")
	bool bKeepJumpLoopWhileAirborne = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump|Transitions")
	bool bTransition_JumpLoopToLand = false;

	/** Emergency: bypass Loop when landed during JumpStart (short-airtime jumps). Wire in AnimBP. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump|Transitions")
	bool bTransition_JumpStartToLand = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump|Transitions")
	bool bTransition_LandToLocomotion = false;

	/** When true, prefer pre-blend loop→land (GetLandPreparationAlpha) before ground contact. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump|Transitions")
	bool bTransition_JumpLoopToLandPrep = false;

	/** Grounded escape from Jump Start/Loop when AnimBP missed Land (wire Loop/Start → Loco). Stays true while grounded. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump|Transitions")
	bool bTransition_JumpGroundedExit = false;

	/** Suggested crossfade durations (seconds) — tune in Class Defaults, read in df.JumpDebug 3. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump|Transitions|Blend", meta = (ClampMin = "0.0"))
	float JumpBlend_LocoToStart = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump|Transitions|Blend", meta = (ClampMin = "0.0"))
	float JumpBlend_StartToLoop = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump|Transitions|Blend", meta = (ClampMin = "0.0"))
	float JumpBlend_LocoToLoop = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump|Transitions|Blend", meta = (ClampMin = "0.0"))
	float JumpBlend_LoopToLand = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump|Transitions|Blend", meta = (ClampMin = "0.0"))
	float JumpBlend_LandToLoco = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump|Transitions|Debug", meta = (ClampMin = "0.0"))
	float JumpLandPrepAlphaThreshold = 0.15f;

	/** Min time in Jump Start before Start→Loop (fallback if anim length unavailable). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump|Transitions|Debug", meta = (ClampMin = "0.0"))
	float JumpStartMinPlayTime = 0.42f;

	/** Force Start→Loop if still in air after this (safety cap for very long start clips). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump|Transitions|Debug", meta = (ClampMin = "0.0"))
	float JumpStartMaxPlayTime = 0.85f;

	/** End stale jump arc when grounded without landing recovery (prevents Arc=1 stuck). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump|Transitions|Debug", meta = (ClampMin = "0.0"))
	float JumpArcGroundedExitTime = 0.12f;

	/** Min rising air time before velocity can latch apex (avoids Vz≈0 flicker on takeoff). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump|Transitions|Debug", meta = (ClampMin = "0.0"))
	float JumpApexMinRisingTime = 0.08f;

	/** Downward Vz (cm/s) required to latch apex after MinRisingTime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump|Transitions|Debug")
	float JumpApexVelocityThreshold = -80.f;

	/** Min grounded time before a new jump arc (filters IsFalling() flicker). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump|Transitions|Debug", meta = (ClampMin = "0.0"))
	float MinGroundedTimeBeforeJump = 0.06f;

	/** Min loop-phase time before Loop->LandPrep (default ≈ Start->Loop blend; avoids prep during crossfade). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump|Transitions", meta = (ClampMin = "0.0"))
	float JumpLoopPrepMinPhaseTime = 0.18f;

	/** Min loop-phase time before Loop->Land on ground contact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump|Transitions", meta = (ClampMin = "0.0"))
	float JumpLoopLandMinPhaseTime = 0.10f;

	/** Min landing recovery before Land->Loco (max with CMC window and land-anim fraction). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump|Transitions", meta = (ClampMin = "0.0"))
	float JumpLandRecoveryMinTime = 0.35f;

	/** Fraction of land anim length used when computing recovery at touchdown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump|Transitions", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float JumpLandRecoveryAnimFraction = 0.45f;

	/** Seconds in loop phase this jump (after start clip ends); debug / AnimBP. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump|Transitions")
	float JumpLoopPhaseTime = 0.f;

	/** True when GAS State.DoubleJumping is active (second jump impulse). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump|Transitions")
	bool bIsDoubleJumping = false;

	/** Set on landing when air time exceeded LongFallAirTimeThreshold. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump|Transitions")
	bool bIsLongFallLanding = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump|Transitions", meta = (ClampMin = "0.0"))
	float LongFallAirTimeThreshold = 1.2f;

	/** Stable rising phase (latched until apex); use for debug / AnimBP. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump|Transitions")
	bool bHasPassedJumpApex = false;

	/** True from takeoff until Land→Locomotion completes (prevents idle Land->Loco spam). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump|Transitions")
	bool bJumpArcActive = false;

	void NotifyLandingRecoveryBegin(float Duration);
	void NotifyLandingRecoveryEnd();

	/** Re-sync jump arc to fall loop after air dash hang (altitude lock freezes Vz). */
	UFUNCTION(BlueprintCallable, Category = "DF|Locomotion|Jump")
	void NotifyAirDashEndedWhileAirborne();

#if !UE_BUILD_SHIPPING
	/** One-line locomotion / blend space state for df.LockOnDebug. */
	FString BuildLocomotionDebugString() const;

	/** Multi-line jump SM transition debug for df.JumpDebug 3 / dump. */
	FString BuildJumpTransitionDebugString() const;

	/** Anim assets, SM elapsed times, montage, blend tuning — df.JumpDebug 4 / dump. */
	FString BuildJumpDeepDebugString();

	void LogJumpDeepSnapshot(const TCHAR* Reason);
#endif

	// ── Default (unarmed) Anim Set ──
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set")
	FUDAnimSet DefaultAnimSet;

	/** Copy updated at init and via Apply/Revert — use Break ActiveAnimSet in AnimGraph for dynamic locomotion BS. */
	UPROPERTY(BlueprintReadOnly, Category = "DF|Anim Set")
	FUDAnimSet ActiveAnimSet;

protected:
	/** Yaw change rate of the actor, used to bank lean into turns. */
	UFUNCTION(BlueprintCallable, Category = "DF|Locomotion")
	void CalculateLean(float DeltaTime);

	/** Control rotation relative to mesh / actor: drives AimOffset node. */
	UFUNCTION(BlueprintCallable, Category = "DF|Locomotion")
	void CalculateAimOffsets();

	/** Fills MovementDirection; use bUseEightWay for 8-wedge vs 4-cardinal. */
	UFUNCTION(BlueprintCallable, Category = "DF|Locomotion")
	void DetermineMovementDirection(bool bUseEightWay = true);

	/** Manual layer link (e.g. debug); item-driven sync uses SyncEquippedWeaponAnimLayerFromOwner. */
	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|Animation")
	void LinkWeaponAnimLayerClass(TSubclassOf<UAnimInstance> AnimLayerClass);

	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|Animation")
	void UnlinkWeaponAnimLayerClass();

	/** Line trace from foot sockets and pelvis: GroundDistance, foot Z offsets. */
	UFUNCTION(BlueprintCallable, Category = "DF|Locomotion")
	void UpdateFootIK(float DeltaTime);

	//~ Cached locomotion
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	TObjectPtr<ACharacter> OwningCharacter;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	TObjectPtr<UDFCharacterMovementComponent> DFCharacterMovement;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	TObjectPtr<UAbilitySystemComponent> OwningAbilitySystem;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	float Speed = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	float Direction = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bIsInAir = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump")
	bool bIsJumping = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump")
	bool bIsFalling = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump")
	bool bIsLanding = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump")
	EDFMovementDirection LastJumpDirection = EDFMovementDirection::None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump")
	float AirTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump")
	float VerticalVelocity = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump")
	float PredictedLandingDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump", meta = (ClampMin = "0.0"))
	float LandPreparationThreshold = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump", meta = (ClampMin = "0.0"))
	float LandPredictionTraceMax = 1000.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bIsDodging = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bIsAirDashing = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	EDFDodgeDirection LastDodgeDirection = EDFDodgeDirection::Backward;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bIsInCombat = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bIsLockedOn = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	float LeanAngle = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Aim")
	float AimPitch = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Aim")
	float AimYaw = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|State")
	bool bIsAttacking = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|State")
	bool bIsCasting = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|State")
	bool bIsStunned = false;

	/** True when Weapon equipment slot has an item (for upper-body / blend poses). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Equipment")
	bool bHasWeaponEquipped = false;

	/** Row name in DT_Items for the equipped weapon; None if unarmed. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Equipment")
	FName EquippedWeaponItemRow = NAME_None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bShouldStrafe = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	EDFMovementDirection MovementDirection = EDFMovementDirection::Forward;

	/** Traced distance from actor to walkable hit below; useful for landing / air blend. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|IK")
	float GroundDistance = 0.f;

	/** Additive world-space style vertical correction to feed into Two-Bone IK (per foot). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|IK")
	float LeftFootHeightOffsetZ = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|IK")
	float RightFootHeightOffsetZ = 0.f;

	/** 0-1: trace hit valid. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|IK")
	float LeftFootIKAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|IK")
	float RightFootIKAlpha = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|IK")
	float FootIK_TraceUp = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|IK")
	float FootIK_TraceDown = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|IK")
	float FootIK_SmoothSpeed = 12.f;

	/** Skeletal names for Mannequin-style rigs; change per character if needed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|IK")
	FName LeftFootSocketName = FName(TEXT("foot_l"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|IK")
	FName RightFootSocketName = FName(TEXT("foot_r"));

	/** Yaw change rate to lean mapping scale (deg lean per world yaw rate). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Lean")
	float LeanFromYawRateScale = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Lean")
	float MaxLeanAngleDeg = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Lean")
	float LeanInterpSpeed = 6.f;

private:
	float LastActorYaw = 0.f;
	bool bLastYawInit = false;
	float LandingRecoveryTimer = 0.f;

	void UpdateJumpTransitionHints();
	void LogJumpTransitionEdges();
	bool IsPrimaryMeshAnimInstance() const;
	void CaptureTakeoffJumpDirection(bool bStrafeForDir);
	float ComputeStartToLoopTime() const;
	bool IsPastApexExitableForJumpLoop() const;
	void PlayFallLoopSlotAfterAirDash();
	void StopFallLoopSlotOverlay();

	UPROPERTY(EditDefaultsOnly, Category = "DF|Locomotion|Jump|AirDash")
	FName FallLoopOverlaySlotName = FName(TEXT("DefaultSlot"));

	bool bPrevTransition_LocoToStart = false;
	bool bPrevTransition_LocoToLoop = false;
	bool bPrevTransition_LoopToLoco = false;
	bool bPrevTransition_StartToLoop = false;
	bool bPrevTransition_StartToLand = false;
	bool bPrevTransition_LoopToLand = false;
	bool bPrevTransition_LandToLoco = false;
	bool bPrevTransition_LoopToLandPrep = false;
	bool bPrevTransition_JumpGroundedExit = false;

	bool bWasInAirPreviousFrame = false;
	bool bWasDoubleJumpingPreviousFrame = false;
	float GroundedTime = 0.f;
	bool bJumpArcEndLatch = false;
	float JumpDeepLogTimer = 0.f;
	/** Resolved at takeoff from GetJumpStartAnim()->GetPlayLength(). */
	float CachedJumpStartPlayTime = 0.42f;
	/** Snapshot at touchdown for Loop->Land gate. */
	float CachedJumpLoopPhaseTimeAtLand = 0.f;
	float AirDashResumeFallLoopLatchTime = 0.f;
	bool bFallLoopOverlayActive = false;
	TObjectPtr<UAnimMontage> FallLoopOverlayMontage = nullptr;

	void SyncEquippedWeaponAnimLayerFromOwner();

	/** Layer class last applied via LinkAnimClassLayers (from item or manual). */
	TSubclassOf<UAnimInstance> CachedLinkedWeaponLayerClass;

	/** Item row whose WeaponAnimSet is currently applied to ActiveAnimSet (NAME_None = unarmed default). */
	FName CachedAnimSetItemRow = NAME_None;

	// Root motion notify stash
	bool bStashedForAnimRoot = false;
	TEnumAsByte<EMovementMode> StashedMovementMode;
	uint8 StashedCustomSubMode = 0;
};
