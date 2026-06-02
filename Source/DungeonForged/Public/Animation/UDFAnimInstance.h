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

	// ── 8-way Start / Loop / Stop locomotion ──
	UFUNCTION(BlueprintPure, Category = "DF|Locomotion|Directional")
	UAnimSequenceBase* GetLocomotionStartAnim() const;

	UFUNCTION(BlueprintPure, Category = "DF|Locomotion|Directional")
	UAnimSequenceBase* GetLocomotionLoopAnim() const;

	UFUNCTION(BlueprintPure, Category = "DF|Locomotion|Directional")
	UAnimSequenceBase* GetLocomotionStopAnim() const;

	/** Grounded idle loop from ActiveAnimSet.IdleAnimation (layer / weapon set picks the clip). */
	UFUNCTION(BlueprintPure, Category = "DF|Locomotion|Idle")
	UAnimSequenceBase* GetLocomotionIdleAnim() const;

	/** Turn-in-place clip for current RootYawOffset (90° or 180°, L/R). Wire Turn state. */
	UFUNCTION(BlueprintPure, Category = "DF|Locomotion|TurnInPlace")
	UAnimSequenceBase* GetLocomotionTurnAnim() const;

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

	/** Multi-line 8-way Start/Loop/Stop locomotion state for df.LocomotionDebug / dump. */
	FString BuildDirectionalLocomotionDebugString() const;

	/** Extra lines: CMC caps, stride scale, loop anim Distance/RM (df.LocomotionDebug 4). */
	FString BuildDirectionalLocomotionDeepDebugString() const;

	/** Turn-in-place only — df.TurnDebug / df.DebugTurnInPlace. */
	FString BuildTurnInPlaceDebugString() const;
	FString BuildTurnInPlaceDebugOneLiner() const;

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

	// ── 8-way Start / Loop / Stop state ──
	/** Snapshot taken on Idle→Start; locked through Start playback. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Directional")
	EDFMovementDirection LocomotionStartDirection = EDFMovementDirection::Forward;

	/** Snapshot taken on Loop→Stop; locked through Stop playback. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Directional")
	EDFMovementDirection LocomotionStopDirection = EDFMovementDirection::Forward;

	/** Gait frozen when Idle→Start fired (used by Get Locomotion Start Anim). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Directional")
	EDFGait LocomotionStartGait = EDFGait::Idle;

	/** Gait frozen when Loop→Stop begins (used by Get Locomotion Stop Anim). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Directional")
	EDFGait LocomotionStopGait = EDFGait::Idle;

	/** Current gait (Idle/Walk/Run/Sprint) based on speed thresholds. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Directional")
	EDFGait Gait = EDFGait::Idle;

	/** True when player is providing input AND velocity is rising or steady. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Directional")
	bool bIsAccelerating = false;

	/** Edge: true on the frame Idle → Start should fire. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Directional")
	bool bTransition_IdleToStart = false;

	/** Edge: true on the frame Start → Loop should fire (Start anim ending or input held long enough). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Directional")
	bool bTransition_StartToLoop = false;

	/** True while decelerating after input release (sustained until stop distance consumed or re-accelerate). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Directional")
	bool bTransition_LoopToStop = false;

	/** Edge: true when Stop finished and speed ~= 0. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Directional")
	bool bTransition_StopToIdle = false;

	/** True when input returns during Stop — wire Stop → Start (or Start/Loop) in the layer SM. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Directional")
	bool bTransition_StopToMove = false;

	/** Cached time since Start was triggered (for fallback Start→Loop). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Directional")
	float LocomotionStartElapsed = 0.f;

	/** Walk threshold (cm/s); below = Idle, between Walk/Run thresholds = Walk. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Directional|Tuning", meta = (ClampMin = "0.0"))
	float WalkSpeedThreshold = 50.f;

	/** Run threshold (cm/s); above this = Run gait set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Directional|Tuning", meta = (ClampMin = "0.0"))
	float RunSpeedThreshold = 350.f;

	/** Idle speed deadband (cm/s); below this counts as stopped for Stop→Idle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Directional|Tuning", meta = (ClampMin = "0.0"))
	float IdleSpeedDeadband = 5.f;

	/** Max time spent in Start before forcing Start→Loop (safety cap). Match authored Start length (~0.83s for Run_Start_F_0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Directional|Tuning", meta = (ClampMin = "0.0"))
	float StartMaxPlayTime = 0.80f;

	// ── Distance Matching ──
	/** Distance accumulated since the last Idle→Start (0 while stopped). Feed into Distance Matching node. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|DistanceMatching")
	float DistanceMatchingDistance = 0.f;

	/** Velocity at takeoff (XY plane). Useful for predicting deceleration distance. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|DistanceMatching")
	float DistanceMatchingStartSpeed = 0.f;

	/** XY distance traveled this frame (cm). Feed Advance Time by Distance Matching on Start (Sequence Evaluator). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|DistanceMatching")
	float DistanceMatchingDelta = 0.f;

	/** Distance remaining until full stop (cm). Feed Distance Match to Target on Stop (Sequence Evaluator). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|DistanceMatching")
	float DistanceMatchingStopToTarget = 0.f;

	/**
	 * Seconds into the active Stop sequence (0 → play length). Wire to Sequence Evaluator → Explicit Time
	 * when the asset has no Distance curve yet, or remove any constant 0 on that pin.
	 */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|DistanceMatching")
	float DistanceMatchingStopExplicitTime = 0.f;

	/** True when the resolved Stop anim has a usable baked "Distance" curve (runtime). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|DistanceMatching")
	bool bActiveStopAnimHasDistanceCurve = false;

	/** Total stop distance on the authored Distance curve (e.g. Run_Stop_F_0: |min curve| ≈ 202). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|DistanceMatching", meta = (ClampMin = "1.0"))
	float AuthoredStopDistance = 202.f;

	/** When true, sets AuthoredStopDistance from default Run stop Distance curve at init (if curve exists). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|DistanceMatching")
	bool bAutoTuneAuthoredStopDistanceFromRunStop = true;

	/**
	 * Once the catch-up is active, drain the remaining StopTarget over this time-constant so the
	 * Stop clip's explicit time advances at a natural rate instead of crawling. Lower = the Stop
	 * animation completes faster (snappier settle); higher = slower. ~0.20 plays close to 1x.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|DistanceMatching", meta = (ClampMin = "0.05"))
	float StopTailCatchUpSeconds = 0.20f;

	/**
	 * Speed (cm/s) below which the Stop tail catch-up engages. MUST be set high enough to cover the
	 * deceleration band, otherwise the Stop clip is purely distance-driven while the capsule glides
	 * slowly and the animation plays in SLOW MOTION (explicit time crawls because little distance is
	 * consumed per frame). The authored Stop clip front-loads its translation and ends in a long
	 * low-distance "settle"; with WalkStopBrakingDeceleration the capsule also stops short of the
	 * clip's full authored distance, so the catch-up is what lets the clip finish on time. Set this
	 * near the run speed (e.g. ~220 for a 429 run) so catch-up covers the whole settle.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|DistanceMatching", meta = (ClampMin = "0.0"))
	float StopTailCatchUpSpeedThreshold = 220.f;

	/** |Distance curve| below this (cm) is treated as end-of-motion for Stop playback (skips flat tail). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|DistanceMatching", meta = (ClampMin = "0.5"))
	float StopCurveNearZeroCm = 8.f;

	// ── Stride Warping ──
	/** Computed alpha for Stride Warping node (1 = full warp, 0 = none). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|StrideWarping")
	float StrideWarpingAlpha = 0.f;

	/** Stride scale (capsule speed / AuthoredLoopSpeed). Feed into the Stride Warping node's Stride Scale pin. 1 = native. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|StrideWarping")
	float StrideScale = 1.f;

	/** Play-rate multiplier for the Loop sequence player to match capsule cadence. Use INSTEAD of StrideScale, not with it. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|StrideWarping")
	float LocomotionPlayRate = 1.f;

	/** Mesh component-space movement direction for the Stride Warping node's Stride Direction pin (Manual mode). Defaults to forward (1,0,0). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|StrideWarping")
	FVector StrideWarpingDirection = FVector(1.f, 0.f, 0.f);

	/** Authored speed of Loop animations (cm/s) — Stride Warping divides actual / authored. Tune to match loop Distance curve (see debug avgSpd~). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|StrideWarping", meta = (ClampMin = "1.0"))
	float AuthoredLoopSpeed = 429.f;

	/** When true, sets AuthoredLoopSpeed from default Run loop Distance curve at init (if curve exists). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|StrideWarping")
	bool bAutoTuneAuthoredLoopSpeedFromRunLoop = true;

	/** Min speed to engage stride warping (avoid warping during deceleration tail). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|StrideWarping", meta = (ClampMin = "0.0"))
	float StrideWarpingMinSpeed = 150.f;

	// ── Turn In Place ──
	/** Accumulated yaw offset (deg) between aim/control rotation and actor — drains via TIP montages. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|TurnInPlace")
	float RootYawOffset = 0.f;

	/** Yaw delta (deg) this frame; sign = direction of rotation. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|TurnInPlace")
	float YawDeltaThisFrame = 0.f;

	/** Should a TIP animation play this frame? (auto: |RootYawOffset| > threshold while idle) */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|TurnInPlace")
	bool bTransition_TurnInPlace = false;

	/** Direction of the queued TIP (positive = right, negative = left). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|TurnInPlace")
	float TurnInPlaceDirection = 0.f;

	/** True while a turn clip is playing (Sequence Evaluator / Turn state). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|TurnInPlace")
	bool bInTurnInPlacePhase = false;

	/** Explicit time (s) for Turn Sequence Evaluator — advanced in C++ each frame. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|TurnInPlace")
	float TurnInPlaceExplicitTime = 0.f;

	/** 90 or 180 for the clip selected this turn (debug / AnimBP). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|TurnInPlace")
	float TurnInPlaceAnimDegrees = 0.f;

	/** Threshold (deg) to trigger a turn in place. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|TurnInPlace", meta = (ClampMin = "0.0"))
	float TurnInPlaceTriggerYaw = 60.f;

	/** |RootYawOffset| at or above this uses Turn_180_* instead of Turn_90_* (latched at turn start). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|TurnInPlace", meta = (ClampMin = "45.0"))
	float TurnInPlace180Threshold = 100.f;

	/**
	 * Rotate the actor yaw while a turn plays. Off = let the Turn sequence drive the pose only
	 * (recommended for Fab clips that already rotate in the animation). On = C++ rotates the pawn
	 * in sync with clip length (use when Enable Root Motion is off and the clip has no body yaw).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|TurnInPlace")
	bool bTurnInPlaceApplyActorYawFromCode = false;

	/** Minimum time (s) in turn phase before bOffsetDone can end early (avoids 1-frame pops). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|TurnInPlace", meta = (ClampMin = "0.0"))
	float TurnInPlaceMinPhaseTime = 0.25f;

	/** After the clip, snap actor yaw for leftover |RootYawOffset| up to this many degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|TurnInPlace", meta = (ClampMin = "0.0"))
	float TurnInPlacePostTurnSnapMaxYaw = 35.f;

	/** |RootYawOffset| must drop below this before another turn can start (anti spam / re-trigger). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|TurnInPlace", meta = (ClampMin = "0.0"))
	float TurnInPlaceRetriggerYaw = 45.f;

	/** While in turn phase, abort only if Speed exceeds this (avoids cancel from end-of-turn rotation / input glitches). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|TurnInPlace", meta = (ClampMin = "0.0"))
	float TurnInPlaceAbortSpeed = 80.f;

	/** Flip L/R clip selection if your Turn_* assets are authored with opposite naming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|TurnInPlace")
	bool bInvertTurnInPlaceDirection = false;

	/** Remaining |RootYawOffset| (deg) below which the turn phase ends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|TurnInPlace", meta = (ClampMin = "0.0"))
	float TurnInPlaceCompleteYaw = 8.f;

	/** Yaw drained per second while moving (bleed offset when not in TIP). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|TurnInPlace", meta = (ClampMin = "0.0"))
	float TurnInPlaceYawInterpSpeed = 360.f;

	// ── Foot Locker / Plant ──
	/** True when the left foot should be locked to its plant point (curve-driven from anim). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|IK")
	bool bLeftFootPlanted = false;

	/** True when the right foot should be locked to its plant point. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|IK")
	bool bRightFootPlanted = false;

	/** Anim curve name fed by Sequence (1 = planted, 0 = free). Default matches Lyra convention. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|IK")
	FName FootPlantCurveLeft = FName(TEXT("FootPlant_L"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|IK")
	FName FootPlantCurveRight = FName(TEXT("FootPlant_R"));

	// ── Aim Offset blend control ──
	/** Alpha 0..1 for AimOffset overlay (1 while strafing / locked-on, 0 free exploration). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|AimOffset")
	float AimOffsetAlpha = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|AimOffset", meta = (ClampMin = "0.0"))
	float AimOffsetInterpSpeed = 8.f;

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
	void UpdateDirectionalLocomotion(float DeltaSeconds);
	void TryAutoTuneAuthoredLoopSpeedFromDefaultRunLoop();
	void TryAutoTuneAuthoredStopDistanceFromDefaultRunStop();
	void UpdateDistanceMatching(float DeltaSeconds);
	void UpdateStrideWarping(float DeltaSeconds);
	void UpdateTurnInPlace(float DeltaSeconds);
	void EnsureActiveAnimSetTurnSet();
	void MergeTurnSetFromLinkedLayersIfEmpty();
	void SyncTurnSetFromPrimaryMeshIfEmpty();
	void DrawTurnInPlaceDebug(const UWorld* World) const;
	void UpdateFootPlantCurves();
	void UpdateAimOffsetBlend(float DeltaSeconds);

	/** Notify TIP montage / state that we consumed N degrees of root yaw offset this tick. */
	UFUNCTION(BlueprintCallable, Category = "DF|Locomotion|TurnInPlace")
	void ConsumeRootYawOffset(float ConsumedYaw);

	/** True when caller (gameplay / lock-on) wants AimOffset overlay active. */
	UFUNCTION(BlueprintCallable, Category = "DF|Locomotion|AimOffset")
	void SetAimOffsetEnabled(bool bEnabled) { bAimOffsetRequested = bEnabled; }

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
	bool bWarnedMissingTurnSet = false;
	float TurnInPlaceYawAppliedTotal = 0.f;
	bool bTurnInPlaceRetriggerArmed = true;
	FString TurnInPlaceLastEndReason;

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

	// Locomotion polish runtime state
	float PreviousActorYaw = 0.f;            // for YawDeltaThisFrame
	float DistanceMatchingAccum = 0.f;        // raw XY distance integrator while moving
	bool bAimOffsetRequested = false;         // gameplay toggles this
	bool bWasAcceleratingPreviousFrame = false; // edge detection for Idle↔Start / Loop→Stop
	bool bWasInLocomotionStopPhasePreviousFrame = false;
	bool bStopToMoveLatch = false;             // sustained Stop→Start while re-accelerating after Stop
	bool bInLocomotionStopPhase = false;       // sustained Loop→Stop until stop match consumed or re-accelerate
	float LocomotionPeakSpeedThisBurst = 0.f;  // max speed while accelerating (stop gait snapshot)
	float LocomotionStopInitialTarget = 0.f;   // StopTarget at input release (for curve time lookup)
	float LocomotionStopDistanceConsumed = 0.f;
	float CachedAuthoredStopPlayLength = 1.5f; // Run_Stop play length for fallback target decay
	float CachedStopMotionEndTime = 1.2f;    // Time where |Distance| curve nears zero (no slow hold tail)
	float CachedTurnAnimPlayLength = 1.f;
	bool bWasInTurnInPlacePhasePreviousFrame = false;
	float TurnInPlaceDebugLogTimer = 0.f;

	void SyncEquippedWeaponAnimLayerFromOwner();

	/** Push 8-way locomotion state from the mesh primary instance to linked layer instances. */
	void CopyDirectionalLocomotionStateFrom(const UUDFAnimInstance& Source);
	void SyncDirectionalLocomotionFromPrimaryMesh();
	void PropagateDirectionalLocomotionToLinkedAnimLayers();

	/** Layer class last applied via LinkAnimClassLayers (from item or manual). */
	TSubclassOf<UAnimInstance> CachedLinkedWeaponLayerClass;

#if !UE_BUILD_SHIPPING
	float LocomotionDebugPrevSpeed = 0.f;
	float LocomotionVerboseLogTimer = 0.f;
#endif

	/** Item row whose WeaponAnimSet is currently applied to ActiveAnimSet (NAME_None = unarmed default). */
	FName CachedAnimSetItemRow = NAME_None;

	// Root motion notify stash
	bool bStashedForAnimRoot = false;
	TEnumAsByte<EMovementMode> StashedMovementMode;
	uint8 StashedCustomSubMode = 0;
};
