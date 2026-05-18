// Source/DungeonForged/Public/Characters/ADFEnemyBase.h
#pragma once

#include "CoreMinimal.h"
#include "Data/DFDataTableStructs.h"
#include "GameFramework/Character.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "UI/Status/UDFStatusLibrary.h"
#include "ADFEnemyBase.generated.h"

class AActor;
class UAbilitySystemComponent;
class UAnimMontage;
class UBlackboardComponent;
class UDataTable;
class UDFAttributeSet;
class UUserWidget;
class UWidgetComponent;
class UDFHitReactionComponent;
class UDFElementalComponent;
class UBehaviorTree;
class UGameplayAbility;
class UGameplayEffect;
class UNiagaraSystem;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDFEnemyDied, AActor*, Enemy, AActor*, Killer, float, ExperienceReward);

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFEnemyBase : public ACharacter, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	ADFEnemyBase();

	//~ AActor
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//~ IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ IGenericTeamAgentInterface
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual void SetGenericTeamId(const FGenericTeamId& InTeamId) override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

	/** Data-driven init (authority / server). Call after spawn, before or after BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "DF|Enemy")
	void InitializeFromDataTable(UDataTable* EnemyTable, FName RowName);

	/** True while o servidor está a esperar que a montagem de spawn termine antes de correr a BT (evita duplo start com @c ADFAIController::OnPossess). */
	UFUNCTION(BlueprintPure, Category = "DF|Enemy")
	bool IsDelayingAIForSpawnBirth() const { return bDeferAIForSpawnBirth; }

	/** NetMulticast: reproduz a montagem de birth em todas as máquinas (chamar só no servidor, típico a partir de `InitializeFromDataTable`). */
	UFUNCTION(NetMulticast, Reliable, Category = "DF|Enemy")
	void Multicast_PlaySpawnBirthMontage(UAnimMontage* Montage);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UDFAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UDFHitReactionComponent> HitReaction;

	/** Data-driven `DT_EnemyElemental` row when `FDFEnemyTableRow::ElementalAffinityRowName` is set. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS|Elemental")
	TObjectPtr<UDFElementalComponent> ElementalComponent;

	/** GAS: Minimal replication = server-focused FX / minimal effect replication to proxies (use for AI). */
	/**
	 * Behavior tree, blackboard, and sight/hearing are owned by ADFAIController.
	 * The asset reference comes from the data row (CachedAIBehaviorTree).
	 */

	/** Cyclic path used by UDFBTTask_FindPatrolPoint. Filled from the enemy row or in Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Patrol")
	TArray<FVector> PatrolPoints;

	/** Random pick for UDFBTTask_PlayTauntMontage. Filled from the enemy data row and/or per-BP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Social")
	TArray<TObjectPtr<UAnimMontage>> TauntMontages;

	/** From enemy data table (or defaults). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI|Combat")
	float MeleeRange = 200.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI|Combat")
	float RangedRange = 2000.f;

	/** UDFBTService_UpdateTarget: proximity / attack “in range” threshold (cm). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI|Combat")
	float AttackRange = 600.f;

	/** 3D / screen-space bar; set Widget class on BP/defaults. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> HealthBar;

	/** 3x debuff row above the health bar; optional. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Status")
	TObjectPtr<UWidgetComponent> DebuffStatusBar;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> HealthBarWidgetClass;

	/** Binds to UDFEnemyDebuffStatusBarWidget. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Status")
	TSubclassOf<UUserWidget> DebuffStatusBarWidgetClass;

	/** If null, status icons use C++ built-in data from UDFStatusLibrary::GetStatusData. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Status")
	TObjectPtr<UDFStatusLibrary> EnemyDebuffStatusLibrary;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Enemy")
	TObjectPtr<UAnimMontage> DeathMontage;

	/** If set, applied once after base stats are written from the data row (e.g. extra “always on” GEs). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Enemy")
	TSubclassOf<UGameplayEffect> OptionalInitGameplayEffect;

	/** Per-class default when the enemy data row has `ElementalAffinityRowName` and no per-row override. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Elemental")
	TObjectPtr<UDataTable> DefaultElementalAffinityTable = nullptr;

	/** Maps gameplay tags (from the enemy data row) to ability classes to grant. Configure on child BP/defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Enemy", meta = (TitleProperty = "GrantedTag"))
	TMap<FGameplayTag, TSubclassOf<UGameplayAbility>> GrantedAbilitiesByTag;

	/** Default team for enemies. Player pawns should use PlayerTeamId (e.g. 1) for attitude checks. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Team")
	uint8 DefaultEnemyTeamId = 2;

	/** Pawns/players to treat as hostile. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Team")
	uint8 DefaultPlayerTeamId = 1;

	UFUNCTION(BlueprintCallable, Category = "AI|Behavior")
	UBlackboardComponent* GetBehaviorTreeBlackboard() const;

	/** Behavior tree from last data-driven init; started in @c ADFAIController::OnPossess or after @a InitializeFromDataTable. */
	UFUNCTION(BlueprintPure, Category = "AI|Behavior")
	UBehaviorTree* GetAIBehaviorTreeAsset() const { return CachedAIBehaviorTree; }

	UPROPERTY(BlueprintAssignable, Category = "DF|Enemy")
	FOnDFEnemyDied OnEnemyDied;

	UFUNCTION(BlueprintPure, Category = "DF|Enemy")
	bool HasDied() const { return bHasDied; }

	UFUNCTION(BlueprintPure, Category = "DF|Enemy")
	float GetCachedExperienceReward() const { return CachedExperienceReward; }

	/** Server-triggered cosmetic cue for enemy abilities. Runs locally on clients; skipped on dedicated server. */
	UFUNCTION(NetMulticast, Unreliable, Category = "DF|Enemy|FX")
	void Multicast_PlayEnemyCosmeticCue(
		USoundBase* Sound,
		UNiagaraSystem* VFX,
		FName AttachSocketName,
		FVector_NetQuantize Location,
		FRotator Rotation,
		FVector_NetQuantize100 Scale,
		bool bAttachToMesh);

	/**
	 * Server: last damaging hit (GAS). Used to credit XP; not replicated.
	 * Call from UDFAttributeSet on incoming health damage, or from tests.
	 */
	void RegisterDamageFromContext(const FGameplayEffectContextHandle& Ctx);

protected:
	void InitAbilityAndBindHealth();
	void UnbindAttributeDelegates();
	void OnHealthOrMaxChanged(float Current, float Max);

	/** Called before death check; boss uses this for phase transitions. */
	virtual void NotifyHealthChangedFromAttributes(float Current, float Max);

	/** Server: runs death flow once, broadcasts, multicasts VFX, schedules destroy. */
	virtual void HandleServerDeath(AActor* Killer);

	UFUNCTION()
	void OnRep_bHasDied();

	/** Applies @a ReplicatedDataTableMaxWalkSpeed to movement (all roles; clients get value via replication from server `InitializeFromDataTable`). */
	UFUNCTION()
	void OnRep_ReplicatedDataTableMaxWalkSpeed();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnDeath(AActor* Killer);

	UFUNCTION(BlueprintNativeEvent, Category = "DF|Enemy")
	void SpawnDeathLoot();
	virtual void SpawnDeathLoot_Implementation();

	UFUNCTION()
	void OnDestroyAfterDeath();

	void ApplyBaseStatsFromRow(const FDFEnemyTableRow& Row);
	void ApplyMovementConfigFromRow(const FDFEnemyTableRow& Row);
	void ApplyAIConfigFromRow(const FDFEnemyTableRow& Row);
	void GrantAbilitiesForRow(const FDFEnemyTableRow& Row);

	/**
	 * Starts @a CachedAIBehaviorTree on the AI controller if both exist.
	 * Needed when @a InitializeFromDataTable runs after @c OnPossess (typical: spawn then init from DT).
	 */
	void TryStartBehaviorTreeFromCache();

	UFUNCTION()
	void OnSpawnBirthMontageDelayElapsed();

	/** Stops the brain, movement, and AI controller input. */
	void DisableEnemyActions();

	void ApplyDeathGameplayState();
	void PlayDeathMontageCosmetic();

	/** Locks the mesh on the final pose so the AnimGraph cannot revert to idle while the actor lingers before destroy. */
	void FinalizeEnemyDeathPose();

	void OnDeathMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
	void OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	FTimerHandle DeathDestroyTimer;
	FTimerHandle DeathPoseLockTimer;

	/** Prevents double montage when multicast and OnRep both run. */
	bool bDeathCosmeticPlayed = false;
	FTimerHandle SpawnBirthBTDelayTimer;

	/** Servidor: quando true, `ADFAIController::OnPossess` não corre a BT — @ref OnSpawnBirthMontageDelayElapsed liberta. */
	bool bDeferAIForSpawnBirth = false;

	UPROPERTY(Transient, DuplicateTransient)
	bool bAttributeDelegatesBound = false;

	/** Replicated to all clients for co-op VFX and UI. Server sets in `HandleServerDeath`. */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_bHasDied, BlueprintReadOnly, Category = "DF|Combat")
	bool bHasDied = false;

	/**
	 * > 0: walk speed from `FDFEnemyTableRow::MaxWalkSpeed` (server `InitializeFromDataTable`). Used so clients
	 * match server movement tuning; 0 = do not override Character defaults from row.
	 */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_ReplicatedDataTableMaxWalkSpeed, BlueprintReadOnly, Category = "DF|Enemy|Movement")
	float ReplicatedDataTableMaxWalkSpeed = 0.f;

	/** Filled in InitializeFromDataTable for loot. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|Enemy")
	TArray<FName> CachedLootTableRowNames;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|Enemy")
	float CachedExperienceReward = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|Enemy|Rewards")
	int32 CachedGoldDropMin = 0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|Enemy|Rewards")
	int32 CachedGoldDropMax = 0;

	/** Server-only: who last applied damage (effect causer / instigator). */
	TWeakObjectPtr<AActor> LastDamageAttacker;

	TObjectPtr<UBehaviorTree> CachedAIBehaviorTree = nullptr;
	FGenericTeamId TeamId;
};
