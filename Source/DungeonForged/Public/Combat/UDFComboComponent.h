// Source/DungeonForged/Public/Combat/UDFComboComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDFComboComponent.generated.h"

class UAnimMontage;
class UDFMeleeTraceComponent;
class UAnimInstance;

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFComboComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFComboComponent();

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo")
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

	/** Filled in BeginPlay if null; or assign in BP. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo")
	TObjectPtr<UDFMeleeTraceComponent> MeleeTrace;

	/** One montage per step (0 .. MaxComboSteps-1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo")
	TArray<TObjectPtr<UAnimMontage>> ComboMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Heavy", meta = (ClampMin = "0.05"))
	float HeavyChargeThreshold = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Heavy", meta = (ClampMin = "1.0"))
	float HeavyDamageMultiplier = 2.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Heavy", meta = (ClampMin = "1.0"))
	float HeavyKnockbackMultiplier = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Heavy", meta = (ClampMin = "0.0"))
	float HeavyStaminaCost = 15.f;

	/** Added to @c UDFMeleeTraceComponent::TraceRadius for the heavy swing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Heavy", meta = (ClampMin = "0.0"))
	float HeavyTraceRadiusBonus = 20.f;

	/** Queues a light attack if pressed during the last moments of a swing montage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo", meta = (ClampMin = "0.0"))
	float AttackInputBufferDuration = 0.15f;

	/** If null, uses equipped-weapon override or first combo montage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Heavy")
	TObjectPtr<UAnimMontage> HeavyAttackMontage;

	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void OnAttackInput();

	/** Hold primary attack: tap on release = light, hold >= @c HeavyChargeThreshold = heavy. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo|Heavy")
	void OnPrimaryAttackPressed();

	UFUNCTION(BlueprintCallable, Category = "Combat|Combo|Heavy")
	void OnPrimaryAttackReleased();

	UFUNCTION(BlueprintPure, Category = "Combat|Combo|Heavy")
	bool IsHeavySwingPending() const { return bHeavySwingPending; }

	/** Server RPC entry for legacy montage-only heavy path (GAS preferred). */
	void ServerCommitHeavyAttack();

	UFUNCTION(BlueprintPure, Category = "Combat|Combo|Heavy")
	UAnimMontage* ResolveHeavyAttackMontage() const;

	/** GAS heavy swing: authority configures trace + schedules windows; sets @c bHeavySwingPending. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo|Heavy")
	void NotifyHeavyAbilitySwingMontageStarted(UAnimMontage* Montage);

	/** Start or restart the chain at step 0. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void StartCombo();

	/** Placed on the timeline where the next chain can start (e.g. AnimNotify AN_ComboWindow). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void AdvanceCombo();

	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void ResetCombo();

	/** Gameplay Ability path: binds montage end + sets bPlayingComboMontage (UDFMeleeTrace + combo window still use the same montage). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void NotifyAbilitySwingMontageStarted(UAnimMontage* Montage);

	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void NotifyAbilitySwingMontagePlaybackEnded();

	/** Exposed for Blueprint; anim delegate calls HandleMontageEndedInternal in C++. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void OnMontageEnded(UAnimMontage* EndedMontage, bool bInterrupted);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnComboWindowTimerExpired();

	void PlayCurrentComboMontage();
	void PrimeMeleeSwingAbilityChain();
	bool ShouldRoutePrimaryMeleeThroughGAS() const;
	bool ShouldRouteHeavyAttackThroughGAS() const;
	bool TryActivatePrimaryMeleeGameplayAbility();
	bool TryActivateHeavyAttackGameplayAbility();
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
	TObjectPtr<UAnimMontage> LastBoundMontageForEnd = nullptr;
	bool bPlayingComboMontage = false;
	bool bHeavySwingPending = false;
	float HeavyChargeStartTime = -1.f;
	bool bSwingInputBuffered = false;
	float SwingInputBufferExpireTime = -1.f;
	void ApplyCombatTuningFromDataAsset();
};
