// Source/DungeonForged/Public/Combat/UDFComboComponent.h
#pragma once

#include "CoreMinimal.h"
#include "AlphaBlend.h"
#include "Components/ActorComponent.h"
#include "Data/DFDataTableStructs.h"
#include "GameplayTagContainer.h"
#include "UDFComboComponent.generated.h"

class UAnimMontage;
class UDFMeleeTraceComponent;
class UAnimInstance;
class UAbilitySystemComponent;
class UGameplayAbility;

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFComboComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFComboComponent();

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Combat|Combo")
	int32 CurrentComboStep = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo", meta = (ClampMin = "1"))
	int32 MaxComboSteps = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo", meta = (ClampMin = "0.0"))
	float ComboWindowDuration = 0.45f;

	/** If true, next attack input in the current window is treated as a chain input. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo")
	bool bComboInputBuffered = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo")
	bool bComboWindowActive = false;

	/** True while the next combo step GA is activating (prevents ResetCombo on the previous swing end). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Combat|Combo")
	bool bComboChainAdvancePending = false;

	/**
	 * Step locked for the next GA activation (survives ResetCombo / PrimeMeleeSwing / OnMontageEnd races).
	 * -1 = use @c CurrentComboStep.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_LockedComboActivationStep, BlueprintReadOnly, Category = "Combat|Combo")
	int32 LockedComboActivationStep = -1;

	/** @deprecated display / server sync mirror; use @c LockedComboActivationStep for activation. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo")
	int32 PendingComboActivationStep = -1;

	/** Step index the next melee GA should play. */
	UFUNCTION(BlueprintPure, Category = "Combat|Combo")
	int32 ResolveComboStepForActivation() const;

	/** Called by the melee GA after it has read the locked step. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void ClearLockedComboStepAfterActivation(int32 ActivatedStep);

	/** Filled in BeginPlay if null; or assign in BP. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo")
	TObjectPtr<UDFMeleeTraceComponent> MeleeTrace;

	/**
	 * One montage per step (0 .. MaxComboSteps-1). The "forward / neutral" path.
	 * Stored as soft refs to keep BP defaults out of the CDO load cascade.
	 * Use @c GetResolvedComboMontage / @c GetComboMontagesResolved at runtime — those read from
	 * the resolved cache populated by @c EnsureBPDefaultMontagesLoaded (async on BeginPlay).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo")
	TArray<TSoftObjectPtr<UAnimMontage>> ComboMontages;

	/** Resolved runtime cache populated by async load or via SetComboMontagesFromHardRefs. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Combat|Combo")
	TArray<TObjectPtr<UAnimMontage>> ResolvedComboMontages;

	/**
	 * Per-step combo data (light + optional heavy finisher branch). When non-empty, overrides montage resolution
	 * for each step and keeps @c ComboMontages in sync with each step's @c LightMontage.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo")
	TArray<FDFComboStep> ComboSteps;

	/** Used when @c IsOwnerAirborne() and this array is non-empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Aerial")
	TArray<FDFComboStep> AerialComboSteps;

	/** Loops while primary attack is held before heavy tier commits (optional). Soft to keep CDO light. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Heavy")
	TSoftObjectPtr<UAnimMontage> ChargeWindupMontage;

	/** Bridge montage on heavy release after windup; falls back to heavy attack montage when null. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Heavy")
	TSoftObjectPtr<UAnimMontage> HeavyChargeReleaseMontage;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Combat|Combo|Heavy")
	TObjectPtr<UAnimMontage> ResolvedChargeWindupMontage;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Combat|Combo|Heavy")
	TObjectPtr<UAnimMontage> ResolvedHeavyChargeReleaseMontage;

	/** True when the next combo step should use @c FDFComboStep::HeavyBranchMontage instead of light. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Combat|Combo")
	bool bComboHeavyFinisherPending = false;

	/**
	 * Optional override per step when the owner is moving backward at the moment the swing starts.
	 * Empty = always use @c ComboMontages.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Directional")
	TArray<TSoftObjectPtr<UAnimMontage>> BackwardComboMontages;

	/** Optional override per step when the owner is strafing left/right. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Directional")
	TArray<TSoftObjectPtr<UAnimMontage>> SideComboMontages;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Combat|Combo|Directional")
	TArray<TObjectPtr<UAnimMontage>> ResolvedBackwardComboMontages;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Combat|Combo|Directional")
	TArray<TObjectPtr<UAnimMontage>> ResolvedSideComboMontages;

	/** Minimum local-axis velocity (cm/s) to consider the swing "directional". Below = neutral/forward. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Directional", meta = (ClampMin = "0.0"))
	float DirectionalInputThreshold = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Heavy", meta = (ClampMin = "0.05"))
	float HeavyChargeThreshold = 0.55f;

	/** Hold past this (≥ @c HeavyChargeThreshold) commits the **max** heavy tier — bigger montage + multipliers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Heavy|MaxTier", meta = (ClampMin = "0.1"))
	float MaxHeavyChargeThreshold = 1.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Heavy", meta = (ClampMin = "1.0"))
	float HeavyDamageMultiplier = 2.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Heavy|MaxTier", meta = (ClampMin = "1.0"))
	float MaxHeavyDamageMultiplier = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Heavy", meta = (ClampMin = "1.0"))
	float HeavyKnockbackMultiplier = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Heavy|MaxTier", meta = (ClampMin = "1.0"))
	float MaxHeavyKnockbackMultiplier = 2.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Heavy", meta = (ClampMin = "0.0"))
	float HeavyStaminaCost = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Heavy|MaxTier", meta = (ClampMin = "0.0"))
	float MaxHeavyStaminaCost = 30.f;

	/** Added to @c UDFMeleeTraceComponent::TraceRadius for the heavy swing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Heavy", meta = (ClampMin = "0.0"))
	float HeavyTraceRadiusBonus = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Heavy|MaxTier", meta = (ClampMin = "0.0"))
	float MaxHeavyTraceRadiusBonus = 35.f;

	/** Queues a light attack if pressed during the last moments of a swing montage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo", meta = (ClampMin = "0.0"))
	float AttackInputBufferDuration = 0.20f;

	/** Extends an open combo window when a melee swing confirms a hit (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo", meta = (ClampMin = "0.0"))
	float ComboRefreshOnHitExtension = 0.30f;

	/** If null, uses equipped-weapon override or first combo montage. Soft to keep CDO light. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Heavy")
	TSoftObjectPtr<UAnimMontage> HeavyAttackMontage;

	/** Played when the player held the button past @c MaxHeavyChargeThreshold. If null, falls back to @c HeavyAttackMontage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Heavy|MaxTier")
	TSoftObjectPtr<UAnimMontage> MaxHeavyAttackMontage;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Combat|Combo|Heavy")
	TObjectPtr<UAnimMontage> ResolvedHeavyAttackMontage;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Combat|Combo|Heavy|MaxTier")
	TObjectPtr<UAnimMontage> ResolvedMaxHeavyAttackMontage;

	/** Blend-in when chaining to the next swing (runtime override via Montage_PlayWithBlendIn). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Combo|Animation", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float ComboChainMontageBlendInTime = 0.12f;

	/** Blend-out when stopping the previous montage during a chain (0 = instant cut). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Combo|Animation", meta = (ClampMin = "0.0", ClampMax = "0.25"))
	float ComboChainMontageStopBlendOutTime = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Combo|Animation")
	EAlphaBlendOption ComboChainBlendOption = EAlphaBlendOption::HermiteCubic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Curve")
	FName ComboWindowCurveName = TEXT("ComboWindow");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Curve", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ComboWindowCurveThreshold = 0.5f;

	/** When true, combo window opens/closes from anim curve; notifies remain fallback when curve is absent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Curve")
	bool bUseCurveInsteadOfNotify = false;

	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void OnAttackInput();

	/** Hold primary attack: tap on release = light, hold >= @c HeavyChargeThreshold = heavy. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo|Heavy")
	void OnPrimaryAttackPressed();

	UFUNCTION(BlueprintCallable, Category = "Combat|Combo|Heavy")
	void OnPrimaryAttackReleased();

	UFUNCTION(BlueprintPure, Category = "Combat|Combo|Heavy")
	bool IsHeavySwingPending() const { return bHeavySwingPending; }

	/** True while a *max* heavy is pending (the highest tier, held past @c MaxHeavyChargeThreshold). */
	UFUNCTION(BlueprintPure, Category = "Combat|Combo|Heavy")
	bool IsMaxHeavyPending() const { return bMaxHeavyPending; }

	/** Called from `ADFPlayerCharacter::Server_NotifyHeavyAttackTier` so server-side GA reads the right tier. */
	void SetMaxHeavyPending(bool bPending) { bMaxHeavyPending = bPending; }

	/** Server RPC entry for legacy montage-only heavy path (GAS preferred). */
	void ServerCommitHeavyAttack();

	UFUNCTION(BlueprintPure, Category = "Combat|Combo|Heavy")
	UAnimMontage* ResolveHeavyAttackMontage() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Combo|Heavy|MaxTier")
	UAnimMontage* ResolveMaxHeavyAttackMontage() const;

	/** Picks the right montage based on which tier is currently pending (max → normal → fallback). */
	UFUNCTION(BlueprintPure, Category = "Combat|Combo|Heavy")
	UAnimMontage* ResolveActiveHeavyMontage() const;

	/** GAS heavy swing: authority configures trace + schedules windows; sets @c bHeavySwingPending. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo|Heavy")
	void NotifyHeavyAbilitySwingMontageStarted(UAnimMontage* Montage);

	/** Start or restart the chain at step 0. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void StartCombo();

	/**
	 * Placed on the timeline where the next chain can start (e.g. AnimNotify AN_ComboWindow).
	 * @param DebugSource Non-shipping log label (e.g. AN_ComboWindowOpen).
	 * @param MontageContext Montage playing when the notify fired (improves df.DebugCombat frame/time).
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void AdvanceCombo(FName DebugSource = NAME_None, UAnimMontage* MontageContext = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void ResetCombo();

	/** Client → server: sync step and run the chain activation on authority (single RPC). */
	UFUNCTION(Server, Reliable)
	void Server_ChainMeleeComboStep(int32 Step);

	/** Gameplay Ability path: binds montage end + sets bPlayingComboMontage (UDFMeleeTrace + combo window still use the same montage). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void NotifyAbilitySwingMontageStarted(UAnimMontage* Montage);

	/** GAS / abilities: montage for @a Step using directional overrides (backward/side) or @c ComboMontages. */
	UFUNCTION(BlueprintPure, Category = "Combat|Combo|Directional")
	UAnimMontage* ResolveDirectionalComboMontage(int32 Step) const;

	/** Populates @c ComboSteps and rebuilds @c ComboMontages from each step's light montage. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void ApplyComboStepData(const TArray<FDFComboStep>& Steps);

	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void ClearComboStepData();

	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void NotifyAbilitySwingMontagePlaybackEnded();

	/** Exposed for Blueprint; anim delegate calls HandleMontageEndedInternal in C++. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void OnMontageEnded(UAnimMontage* EndedMontage, bool bInterrupted);

	/** Effective chain cap: min(MaxComboSteps, ComboMontages.Num()). */
	UFUNCTION(BlueprintPure, Category = "Combat|Combo")
	int32 GetEffectiveMaxComboSteps() const;

	/** Mid-chain melee swings skip Ability.Cooldown apply/check (last step still uses CD). */
	UFUNCTION(BlueprintPure, Category = "Combat|Combo|GAS")
	bool ShouldBypassMeleeAbilityCooldown() const;

	/** Extends the active combo window after a confirmed hit (defaults to @c ComboRefreshOnHitExtension). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void NotifyOwnerHitConfirmed(float ExtensionSeconds = -1.f);

	/** Opens a tag-filtered window where other abilities may cancel in (see @c UANS_DFAbilityCancelWindow). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo|Cancel")
	void SetAbilityCancelWindow(const FGameplayTagContainer& AllowedCancelTags);

	UFUNCTION(BlueprintCallable, Category = "Combat|Combo|Cancel")
	void ClearAbilityCancelWindow();

	UFUNCTION(BlueprintPure, Category = "Combat|Combo|Cancel")
	bool IsAbilityCancelWindowActive() const { return bAbilityCancelWindowActive; }

	/** True when @a AbilityTags overlap @c AllowedAbilityCancelTags during an open cancel window. */
	UFUNCTION(BlueprintPure, Category = "Combat|Combo|Cancel")
	bool IsAbilityCancellable(const FGameplayTagContainer& AbilityTags) const;

	/** True while an ability cancel notify window is open (jump-cancel, dodge-cancel, etc.). */
	UFUNCTION(BlueprintPure, Category = "Combat|Combo|Cancel")
	bool IsInCancelWindow() const;

	/** True when aerial combo steps are configured for this weapon. */
	UFUNCTION(BlueprintPure, Category = "Combat|Combo|Aerial")
	bool HasAerialContinuation() const;

	/** Stops the active attack montage without resetting combo counters. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo|Cancel")
	void CancelCurrentMontage();

	/** Resets combo after @a GraceSeconds unless a new swing chains in the meantime. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void RequestDeferredReset(float GraceSeconds = 0.35f);

	UFUNCTION(BlueprintPure, Category = "Combat|Combo")
	float ResolveChainBlendInForStep(int32 Step) const;

	UFUNCTION(BlueprintPure, Category = "Combat|Combo")
	EAlphaBlendOption ResolveChainBlendOptionForStep(int32 Step) const;

	/** Seconds to skip at the start of step @a Step's montage when chained. 0 = no skip. */
	UFUNCTION(BlueprintPure, Category = "Combat|Combo")
	float ResolveChainStartOffsetForStep(int32 Step) const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	UAnimMontage* PickComboVariant(const TArray<FDFComboVariant>& Variants) const;

	UFUNCTION(BlueprintPure, Category = "Combat|Combo|Aerial")
	bool IsOwnerAirborne() const;

	/** Active step data (aerial vs grounded). */
	UFUNCTION(BlueprintPure, Category = "Combat|Combo")
	bool GetActiveComboStep(int32 Step, FDFComboStep& OutStep) const;

#if !UE_BUILD_SHIPPING
	/** Used by melee GA + df.DebugCombat overlay. */
	void RecordChainMontageBlendIn(float RuntimeBlendIn, UAnimMontage* Montage = nullptr);
#endif

	/** Sends finisher QTE input when Execute is active; tries Execute when FinisherReady. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo|Finisher")
	bool TryHandleFinisherPrimaryInput();

	// ─── Soft-ref runtime API ──────────────────────────────────────────────────────────────
	// Runtime (weapon-equip / class fallback) supplies hard refs from DataTable. These helpers
	// convert to TSoftObjectPtr storage *and* populate the Resolved* caches in one shot, so
	// callers don't have to wait for async load when the assets are already in memory.

	/** Populates @c ComboMontages (soft) AND @c ResolvedComboMontages (hard) from a hard-ref array. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void SetComboMontagesFromHardRefs(const TArray<UAnimMontage*>& Hards);

	UFUNCTION(BlueprintCallable, Category = "Combat|Combo|Directional")
	void SetBackwardComboMontagesFromHardRefs(const TArray<UAnimMontage*>& Hards);

	UFUNCTION(BlueprintCallable, Category = "Combat|Combo|Directional")
	void SetSideComboMontagesFromHardRefs(const TArray<UAnimMontage*>& Hards);

	// C++-only overloads (UHT rejects TObjectPtr in UFUNCTION parameters).
	void SetComboMontagesFromHardRefs(const TArray<TObjectPtr<UAnimMontage>>& Hards);
	void SetBackwardComboMontagesFromHardRefs(const TArray<TObjectPtr<UAnimMontage>>& Hards);
	void SetSideComboMontagesFromHardRefs(const TArray<TObjectPtr<UAnimMontage>>& Hards);

	/**
	 * Equip-path setters: receive soft refs from DataTable rows. Performs sync load on each entry
	 * (one-shot cost at equip — keeps combat hot path on cache reads only). Empty refs are preserved as null.
	 */
	void SetComboMontagesFromSoftRefs(const TArray<TSoftObjectPtr<UAnimMontage>>& Softs);
	void SetBackwardComboMontagesFromSoftRefs(const TArray<TSoftObjectPtr<UAnimMontage>>& Softs);
	void SetSideComboMontagesFromSoftRefs(const TArray<TSoftObjectPtr<UAnimMontage>>& Softs);
	void SetHeavyAttackMontageFromSoft(const TSoftObjectPtr<UAnimMontage>& Soft);
	void SetMaxHeavyAttackMontageFromSoft(const TSoftObjectPtr<UAnimMontage>& Soft);
	void SetChargeWindupMontageFromSoft(const TSoftObjectPtr<UAnimMontage>& Soft);
	void SetHeavyChargeReleaseMontageFromSoft(const TSoftObjectPtr<UAnimMontage>& Soft);

	UFUNCTION(BlueprintCallable, Category = "Combat|Combo|Heavy")
	void SetHeavyAttackMontage(UAnimMontage* Montage);

	UFUNCTION(BlueprintCallable, Category = "Combat|Combo|Heavy|MaxTier")
	void SetMaxHeavyAttackMontage(UAnimMontage* Montage);

	UFUNCTION(BlueprintCallable, Category = "Combat|Combo|Heavy")
	void SetChargeWindupMontage(UAnimMontage* Montage);

	UFUNCTION(BlueprintCallable, Category = "Combat|Combo|Heavy")
	void SetHeavyChargeReleaseMontage(UAnimMontage* Montage);

	/** Snapshot of all soft refs (BP defaults snapshot for restore by ADFPlayerCharacter). */
	struct FComboMontageBaselineSnapshot
	{
		TArray<TSoftObjectPtr<UAnimMontage>> Combo;
		TArray<TSoftObjectPtr<UAnimMontage>> Backward;
		TArray<TSoftObjectPtr<UAnimMontage>> Side;
		TSoftObjectPtr<UAnimMontage> Heavy;
		TSoftObjectPtr<UAnimMontage> MaxHeavy;
		TSoftObjectPtr<UAnimMontage> ChargeWindup;
		TSoftObjectPtr<UAnimMontage> HeavyChargeRelease;
	};

	void CaptureBaselineSnapshot(FComboMontageBaselineSnapshot& OutSnapshot) const;
	void RestoreFromBaselineSnapshot(const FComboMontageBaselineSnapshot& Snapshot);

	/** Triggers async load of all soft refs and fills Resolved* caches on completion. */
	void EnsureBPDefaultMontagesLoaded();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_LockedComboActivationStep();

	UFUNCTION()
	void OnComboWindowTimerExpired();

	void EvaluateComboCurveWindow();
	UAnimMontage* ResolveStepMontageFromData(const FDFComboStep& StepData) const;

	void PlayCurrentComboMontage();
	void PrimeMeleeSwingAbilityChain();
	bool ShouldRoutePrimaryMeleeThroughGAS() const;
	bool ShouldRouteHeavyAttackThroughGAS() const;
	bool TryActivatePrimaryMeleeGameplayAbility();
	bool TryActivateHeavyAttackGameplayAbility();
#if !UE_BUILD_SHIPPING
	void LogPrimaryMeleeActivateFailure(UAbilitySystemComponent* ASC, TSubclassOf<UGameplayAbility> AbilityClass) const;
#endif
	void UnbindMontageEndDelegate();
	void HandleMontageEndedInternal(class UAnimMontage* EndedMontage, bool bInterrupted);
	UAnimInstance* GetAnimInstance() const;
	void TryBindEndDelegateFor(UAnimMontage* Montage);
	bool CanPerformHeavyAttack() const;
	void CommitHeavyAttack();
	void ExecuteHeavyAttackPresentation();
	void ExecuteHeavyAttackAuthority();
	bool ConsumeHeavyStamina();

	FTimerHandle ComboWindowTimer;
	FTimerHandle DeferredResetTimer;

	UFUNCTION()
	void OnDeferredResetTimer();
	TObjectPtr<UAnimMontage> LastBoundMontageForEnd = nullptr;
	bool bPlayingComboMontage = false;
	/** Set while stopping a montage for GAS chain; blocks BufferedSwingChain on intentional interrupt. */
	bool bSuppressBufferedSwingChainOnMontageEnd = false;
	bool bHeavySwingPending = false;
	bool bMaxHeavyPending = false;
	float HeavyChargeStartTime = -1.f;
	float ComboBranchPressTime = -1.f;
	bool bSwingInputBuffered = false;
	float SwingInputBufferExpireTime = -1.f;
	float ComboWindowExpireTime = -1.f;
#if !UE_BUILD_SHIPPING
	FString LastComboWindowOpenSource;
	float LastComboWindowOpenWorldTime = -1.f;
	float LastComboWindowOpenMontageTime = -1.f;
	int32 LastComboWindowOpenMontageFrame = -1;
	FString LastComboWindowOpenMontageName;
#endif
	bool bPlayingChargeWindup = false;
#if !UE_BUILD_SHIPPING
	float LastChainRuntimeBlendIn = -1.f;
#endif
	bool bPendingChargeReleaseMontage = false;
	bool bAbilityCancelWindowActive = false;
	FGameplayTagContainer AllowedAbilityCancelTags;
	bool bSwingHitConfirmedThisActivation = false;
	int32 LastRepLockedComboStep = -1;
	void ApplyCombatTuningFromDataAsset();
	void BufferComboInputAndTryAdvance();
	void DrawCombatDebug() const;
	void StartChargeWindupMontage();
	void StopChargeWindupMontage();
	void TryAdvanceComboBranchFromHold();
	bool IsInputBufferExpired(float ExpireGameTime) const;
	void ArmComboWindowTimer();
#if !UE_BUILD_SHIPPING
	void RecordComboWindowOpened(FName Source, UAnimMontage* MontageAtNotify = nullptr);
	void RecordComboWindowClosed(FName Source);
#endif
	/** Stops the previous swing montage and clears end delegates before a GAS combo chain step. */
	void PrepareForComboChainActivation();

	void CommitMaxHeavyAttack();
	bool CanPerformMaxHeavyAttack() const;
	bool ConsumeMaxHeavyStamina();
	void ExecuteMaxHeavyAttackAuthority();
	void ExecuteMaxHeavyAttackPresentation();

	// Repopulates Resolved* caches by reading currently-loaded soft pointers.
	void RefreshResolvedMontageCaches();

	/** Streamable handle for the async load kicked off in @c EnsureBPDefaultMontagesLoaded. */
	TSharedPtr<struct FStreamableHandle> DefaultMontagesStreamHandle;
};
