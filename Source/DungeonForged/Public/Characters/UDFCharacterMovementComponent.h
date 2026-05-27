// Source/DungeonForged/Public/Characters/UDFCharacterMovementComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Combat/DFDodgeTypes.h"
#include "GameplayTagContainer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/WeakObjectPtr.h"
#include "UDFCharacterMovementComponent.generated.h"

class ACharacter;
class FNetworkPredictionData_Client;
class FNetworkPredictionData_Client_Character;

DECLARE_MULTICAST_DELEGATE_ThreeParams(
	FOnUDFMovementModeChanged,
	EMovementMode /* NewMovementMode */,
	EMovementMode /* PreviousMovementMode */,
	uint8 /* PreviousCustomMode */);

/**
 * Project CharacterMovement: default ground speed (RunSpeed), faster sprint (ability + hold input),
 * stamina drain, dodge root motion, movement-mode anim notify.
 * FSavedMove_DF: network prediction bWantsSprint in FLAG_Custom_0.
 */
UCLASS()
class DUNGEONFORGED_API UDFCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UDFCharacterMovementComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Max ground speed while not sprinting (default locomotion / "run"). */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement", meta = (FormerlySerializedAs = "WalkSpeed", ClampMin = "0.0"))
	float RunSpeed = 540.f;

	/** Max ground speed while sprint ability is active (typically hold sprint). */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement", meta = (ClampMin = "0.0"))
	float SprintSpeed = 750.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement")
	float CrouchSpeed = 200.f;

	/** Stamina / second when sprinting (numeric drain). Ignored if bSprintStaminaFromGameplayEffect is true. */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Sprint", meta = (ClampMin = "0.0"))
	float SprintStaminaDrain = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Dodge", meta = (ClampMin = "0.0"))
	float DodgeCooldown = 0.7f;

	/** World-space dodge displacement magnitude (cm) over DodgeDuration. */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Dodge", meta = (ClampMin = "0.0"))
	float DodgeDistance = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Dodge", meta = (ClampMin = "0.0"))
	float DodgeDuration = 0.35f;

	/** i-frames: State.Invulnerable window (may be shorter than DodgeDuration). */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Dodge", meta = (ClampMin = "0.0"))
	float IFrameDuration = 0.35f;

	UPROPERTY(BlueprintReadOnly, Category = "DF|Movement|Sprint")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Category = "DF|Movement|Dodge")
	bool bIsDodging = false;

	/** Last cardinal dodge direction (set by UDFAbility_Dodge before impulse). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Movement|Dodge")
	EDFDodgeDirection LastDodgeDirection = EDFDodgeDirection::Backward;

	/** When true, stamina drain is handled by a periodic GameplayEffect (Sprint ability) instead of TickSprintStamina. */
	UPROPERTY(Transient)
	bool bSprintStaminaFromGameplayEffect = false;

	/** Optional: applied when Stamina runs out while sprinting (CMC or GE drain). */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Sprint")
	TSubclassOf<class UGameplayEffect> SprintExhaustionEffect;

	/** Binds in AnimInstance / BP to react to movement mode changes. */
	FOnUDFMovementModeChanged OnDFMovementModeChanged;

	/** bFromNetworkPrediction: used when unpacking ServerMove compressed flags; does not change behavior beyond sync. */
	void SetSprinting(bool bSprint, bool bFromNetworkPrediction = false);
	void SetSprintStaminaFromGameplayEffect(bool bFromEffect);

	/** If not using periodic GE, drains SprintStaminaDrain * dt from the owner's ASC. At 0 stamina, stops sprint and applies optional exhaustion. */
	void TickSprintStamina(float DeltaTime);

	/**
	 * GAS dodge: tags, cooldown, timers. Optional programmatic MoveToForce (off when montage supplies root motion).
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|Movement|Dodge", meta = (AdvancedDisplay = "bApplyProgrammaticDisplacement"))
	void PerformDodge(const FVector& DirectionWorld, bool bApplyProgrammaticDisplacement = true);

	/** Last movement input in world; if nearly zero, returns -Actor forward (backward). */
	UFUNCTION(BlueprintCallable, Category = "DF|Movement|Dodge")
	FVector GetDodgeDirection() const;

	/** Seconds until PerformDodge accepts another impulse (0 if ready). */
	UFUNCTION(BlueprintPure, Category = "DF|Movement|Dodge")
	float GetDodgeCooldownRemaining() const;

	/** Seconds until another air dash is allowed (0 if ready). Combined with the per-jump latch. */
	UFUNCTION(BlueprintPure, Category = "DF|Movement|Jump|AirDash")
	float GetAirDashCooldownRemaining() const;

	/** Strafe (lock-on): face controller yaw; exploration: orient to movement. */
	UFUNCTION(BlueprintCallable, Category = "DF|Movement|Strafe")
	void SetStrafeMode(bool bStrafe);

	UPROPERTY(BlueprintReadOnly, Category = "DF|Movement|Strafe")
	bool bIsStrafing = false;

	// ── Jump tuning ─────────────────────────────────────────────────────
	// Defaults below mirror UDFCombatTuningData; tuning asset (DA_CombatTuning) overrides at BeginPlay
	// via ApplyJumpTuningFromDataAsset(). Keep both in sync to avoid editor / runtime mismatch.
	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "0.0"))
	float DFJumpZVelocity = 750.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DFAirControl = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "0.0"))
	float DFGravityScale = 1.7f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "1.0"))
	float DFFallGravityMultiplier = 1.25f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "0.0"))
	float DFJumpStaminaCost = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "0.0"))
	float DFJumpCooldown = 0.20f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "0.0"))
	float DFLandingRecoveryWindow = 0.20f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float CoyoteTime = 0.10f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float JumpApexCutScale = 0.40f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "1.0"))
	float SprintJumpHorizontalBoost = 1.25f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "0.0"))
	float JumpBufferGroundDistance = 250.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "0.0"))
	float DFDoubleJumpStaminaCost = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DFDoubleJumpZScale = 0.85f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump|AirDash", meta = (ClampMin = "0.0"))
	float AirDashDistance = 520.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump|AirDash", meta = (ClampMin = "0.0"))
	float AirDashDuration = 0.22f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump|AirDash", meta = (ClampMin = "0.0"))
	float AirDashCooldown = 0.40f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump|AirDash", meta = (ClampMin = "0.0"))
	float AirDashLandingRecoverySkipWindow = 0.50f;

	/**
	 * Horizontal velocity retained on touch-down (0..1). At 0.4 the character keeps 40% of
	 * its airborne XY speed at landing — the rest is shed instantly to stop "sliding".
	 * Set to 1.0 to disable (preserve momentum, classic UE feel).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump|Landing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LandingHorizontalVelocityRetain = 0.4f;

	/**
	 * Braking deceleration applied while State.Landing tag is active.
	 * Default UE value is 2048; bumping this to 4096 cuts the stop time in half.
	 * Only active during the landing recovery window; reverts to normal afterwards.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump|Landing", meta = (ClampMin = "0.0"))
	float LandingBrakingDeceleration = 4096.f;

	/** One air dodge per jump arc; cleared on landing. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Movement|Jump")
	bool bAirDodgeUsedThisJump = false;

	/** Stash for AnimNotifyState_AerialHangtime (not replicated). */
	float AerialHangtimeSavedGravity = -1.f;

	UFUNCTION(BlueprintPure, Category = "DF|Movement|Jump")
	float GetJumpCooldownRemaining() const;

	UFUNCTION(BlueprintPure, Category = "DF|Movement|Jump")
	bool IsWithinCoyoteWindow() const;

	UFUNCTION(BlueprintPure, Category = "DF|Movement|Jump")
	bool IsFallingNearGround(float MaxGroundDistance = -1.f) const;

	/** Called by UDFAbility_AirDash when a dash starts (tracks landing recovery skip). */
	void NotifyAirDashPerformed();

	/** Keeps world Z flat during air dash (strips anim RM vertical drift). Cleared when dash ends. */
	void BeginAirDashAltitudeLock(float LockedWorldZ);
	void ClearAirDashAltitudeLock();

	bool IsAirDashAltitudeLocked() const { return bAirDashAltitudeLockActive; }

	/**
	 * Authoritative air-dash displacement: locks Z, zeroes gravity/acceleration, drives XY velocity for Duration.
	 * Movement stays independent of montage root motion (montage is visual-only).
	 */
	void BeginAirDashDrive(const FVector& DirWorld, float Distance, float Duration, float LockedWorldZ);

	/** Ends drive, restores acceleration, optionally retains a fraction of dash XY speed. */
	void EndAirDashDrive(float ExitVelocityRetain = 0.15f);

	/** Stops horizontal impulse but keeps altitude lock (hang until ability ends). */
	void EndAirDashDriveImpulse();

	bool IsAirDashDriveActive() const { return bAirDashDriveActive; }

	/** Fraction of dash XY speed kept when the dash ends (0 = stop dead, 1 = full momentum). */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump|AirDash", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AirDashExitVelocityRetain = 0.15f;

	/** Public entry for coyote / double-jump when ACharacter::Jump cannot reach protected DoJump. */
	bool RequestJump(bool bReplayingMoves = false);

	/** Clears stale Jumping/Falling; removes Landing after recovery window. Call before jump input. */
	void SyncJumpLooseTagsWhileGrounded(class UAbilitySystemComponent* ASC);

private:
	float NormalBrakingDecelerationWalking = -1.f;  // cached on first OnMovementModeChanged
	bool bIsApplyingLandingBrake = false;

	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual bool DoJump(bool bReplayingMoves) override;

protected:
	virtual void BeginPlay() override;

	float DefaultBrakingFrictionFactor = 1.f;

	float TimeLastDodge = -1.f;
	float TimeLastJump = -1.f;
	/** World time when we last landed (Falling → Walking). */
	float TimeLastLanded = -1.f;
	/** Ledge drop (not jump takeoff) — enables coyote window. */
	float TimeLastLeftGround = -1.f;
	float TimeLastAirDash = -1.f;
	bool bAirDashAltitudeLockActive = false;
	float AirDashLockedWorldZ = 0.f;
	bool bAirDashDriveActive = false;
	FVector AirDashDriveDir = FVector::ZeroVector;
	float AirDashDriveSpeed = 0.f;
	float AirDashDriveEndTime = -1.f;
	float SavedAirDashMaxAcceleration = 0.f;
	bool bCoyoteFromLedgeDrop = false;

	bool TryConsumeStaminaForJumpCost(float Cost) const;
	void ApplySprintJumpMomentumBoost();
	FTimerHandle TimerHandle_EndDodging;
	FTimerHandle TimerHandle_EndIFrame;
	FTimerHandle TimerHandle_EndLanding;

	void ApplyJumpTuningFromDataAsset();

	/** Sets loose tag count to 0 (RemoveLooseGameplayTag Tag,0 removes nothing in UE5). */
	void ClearLooseGameplayTagAll(class UAbilitySystemComponent* ASC, const FGameplayTag& Tag) const;

	/** Removes every stack of airborne jump tags (Jumping / Falling). */
	void ClearJumpAirborneLooseTags(class UAbilitySystemComponent* ASC) const;

	void ClearJumpLandingLooseTag(class UAbilitySystemComponent* ASC) const;

	/** Adds a loose tag only if not already present (prevents xN stacks). */
	void AddJumpLooseTagOnce(class UAbilitySystemComponent* ASC, const FGameplayTag& Tag) const;

	/** True after apex: State.Falling applied, State.Jumping cleared. */
	bool bJumpFallingTagActive = false;

	UFUNCTION()
	void EndDodgingState();

	UFUNCTION()
	void EndIFrameState();

	/** Called on drain-path exhaustion. */
	void ApplySprintExhaustionIfAny();

	void RefreshMaxWalkSpeed();
};

struct FSavedMove_DF : public FSavedMove_Character
{
public:
	bool bWantsSprint = false;

	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual void SetMoveFor(
		ACharacter* C,
		float InDeltaTime,
		FVector const& NewAccel,
		FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* C) override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
};

class FNetworkPredictionData_DF : public FNetworkPredictionData_Client_Character
{
public:
	explicit FNetworkPredictionData_DF(const UCharacterMovementComponent& ClientMovement);
	virtual FSavedMovePtr AllocateNewMove() override;
};
