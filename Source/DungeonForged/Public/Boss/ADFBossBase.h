// Source/DungeonForged/Public/Boss/ADFBossBase.h
#pragma once

#include "CoreMinimal.h"
#include "Characters/ADFEnemyBase.h"
#include "ADFBossBase.generated.h"

class UAnimMontage;
class UCameraShakeBase;
class UNiagaraSystem;
class UDFGameplayAbility;
class UGameplayEffect;
class AActor;
class UNiagaraComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDFBossPhaseChanged, int32, OldPhase, int32, NewPhase, AActor*, Boss);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDFBossEnraged, AActor*, Boss, bool, bEnraged);

/**
 * Phased boss: HP thresholds, per-phase ability grants, enrage timer, GAS + replication.
 * PhaseAbilities: index 0 = extra ability for phase 2, index 1 = for phase 3, etc. (1-based NewPhase-2).
 */
UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFBossBase : public ADFEnemyBase
{
	GENERATED_BODY()
public:
	ADFBossBase();

	//~ AActor
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Server: transition; applies stun, stats, VFX, grants, multicast FX. */
	UFUNCTION(BlueprintCallable, Category = "DF|Boss", meta = (AutoCreateRefTerm = "NewPhase"))
	void TriggerPhaseTransition(int32 NewPhase);

	/** Server: enrage from timer. */
	UFUNCTION(BlueprintCallable, Category = "DF|Boss")
	void OnEnrageTimerExpired();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_BossCurrentPhase, Category = "DF|Boss|Phase")
	int32 CurrentPhase = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Boss|Phase", meta = (ClampMin = "1"))
	int32 MaxPhases = 3;

	/** When Health/Max first drops to or below these ratios, advance phase (e.g. 0.6, 0.3 for two transitions to phases 2 and 3). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Boss|Phase", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	TArray<float> PhaseThresholds = {0.6f, 0.3f};

	/** Unlocks when entering phase (index = NewPhase - 2; phase 2 uses [0], phase 3 uses [1], …). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Boss|Phase")
	TArray<TSubclassOf<UDFGameplayAbility>> PhaseAbilities;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Boss|Cinematic")
	TObjectPtr<UAnimMontage> PhaseTransitionMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Boss|Cinematic")
	TObjectPtr<UNiagaraSystem> PhaseTransitionVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Boss|Cinematic")
	TObjectPtr<UAnimMontage> EnrageRoarMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Boss|Cinematic")
	TObjectPtr<UNiagaraSystem> EnrageVFX;

	/** Tuning: seconds before enrage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Boss|Enrage", meta = (ClampMin = "0.0"))
	float EnrageTimer = 120.f;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BossEnraged, Category = "DF|Boss|Enrage")
	bool bIsEnraged = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Boss|GAS")
	TSubclassOf<UGameplayEffect> StunForPhaseTransition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Boss|GAS")
	TSubclassOf<UGameplayEffect> PhaseStatEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Boss|GAS")
	TSubclassOf<UGameplayEffect> EnrageEffect;

	/** 1.5s self stun (SetByCaller Data.Duration). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Boss|GAS", meta = (ClampMin = "0.0"))
	float PhaseStunDuration = 1.5f;

	UPROPERTY(BlueprintAssignable, Category = "DF|Boss")
	FOnDFBossPhaseChanged OnBossPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "DF|Boss")
	FOnDFBossEnraged OnBossEnraged;

	/** Cinematic; defaults to WBP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Boss|UI")
	FText BossDisplayName;

	UFUNCTION(BlueprintCallable, Category = "DF|Boss|UI")
	FText GetBossDisplayName() const { return BossDisplayName; }

	/** Abilities (slam, charge) call this; shake/Niagara on ability; inner/outer in cm. */
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_BossLocalAttackFX(
		const FVector& Origin,
		TSubclassOf<UCameraShakeBase> CameraShake,
		UNiagaraSystem* Niagara = nullptr,
		float CameraShakeInnerRadius = 0.f,
		float CameraShakeOuterRadius = 2000.f);

	/** Replicated to all clients: short montage (e.g. charge stumble) on the boss mesh. */
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayLocalMontage(UAnimMontage* Montage, float PlayRate, FName StartSectionName = NAME_None);

protected:
	virtual void NotifyHealthChangedFromAttributes(float Current, float Max) override;
	virtual void HandleServerDeath(AActor* Killer) override;

	UFUNCTION()
	void OnRep_BossCurrentPhase();

	UFUNCTION()
	void OnRep_BossEnraged();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnPhaseTransitionVFX();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnEnrageVFX();

	void TryAdvancePhaseFromHealth(float Current, float Max);
	void ClearSpawnedMinions();

	FTimerHandle EnrageTimerHandle;
	bool bEnrageTimerSet = false;

	/** Replicated current phase; used on clients in OnRep to build Old for OnBossPhaseChanged. */
	int32 LocalPhaseCache = 1;
	bool bLocalEnragedCache = false;

	/** Tracked for cleanup (weak). */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ADFEnemyBase>> SpawnedMinions;

public:
	/** Abilities call to track minion cap; binds death to clear from list. */
	UFUNCTION(BlueprintCallable, Category = "DF|Boss|Minions")
	void RegisterSpawnedMinion(ADFEnemyBase* Minion);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DF|Boss|Minions")
	int32 GetLivingMinionCount() const;

protected:
	UFUNCTION()
	void HandleMinionEnemyDied(AActor* Enemy, AActor* Killer, float Exp);
};
