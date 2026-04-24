// Source/DungeonForged/Public/Characters/ADFEnemyBase.h
#pragma once

#include "CoreMinimal.h"
#include "Data/DFDataTableStructs.h"
#include "GameFramework/Character.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
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
class UBehaviorTree;
class UGameplayAbility;
class UGameplayEffect;

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

	//~ IGenericTeamAgentInterface
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual void SetGenericTeamId(const FGenericTeamId& InTeamId) override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

	/** Data-driven init (authority / server). Call after spawn, before or after BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "DF|Enemy")
	void InitializeFromDataTable(UDataTable* EnemyTable, FName RowName);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UDFAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UDFHitReactionComponent> HitReaction;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> HealthBarWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Enemy")
	TObjectPtr<UAnimMontage> DeathMontage;

	/** If set, applied once after base stats are written from the data row (e.g. extra “always on” GEs). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Enemy")
	TSubclassOf<UGameplayEffect> OptionalInitGameplayEffect;

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

	/** Behavior tree from last data-driven init; ADFAIController::OnPossess runs it. */
	UFUNCTION(BlueprintPure, Category = "AI|Behavior")
	UBehaviorTree* GetAIBehaviorTreeAsset() const { return CachedAIBehaviorTree; }

	UPROPERTY(BlueprintAssignable, Category = "DF|Enemy")
	FOnDFEnemyDied OnEnemyDied;

	UFUNCTION(BlueprintPure, Category = "DF|Enemy")
	bool HasDied() const { return bHasDied; }

	UFUNCTION(BlueprintPure, Category = "DF|Enemy")
	float GetCachedExperienceReward() const { return CachedExperienceReward; }

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

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnDeath(AActor* Killer);

	UFUNCTION(BlueprintNativeEvent, Category = "DF|Enemy")
	void SpawnDeathLoot();
	virtual void SpawnDeathLoot_Implementation();

	UFUNCTION()
	void OnDestroyAfterDeath();

	void ApplyBaseStatsFromRow(const FDFEnemyTableRow& Row);
	void ApplyAIConfigFromRow(const FDFEnemyTableRow& Row);
	void GrantAbilitiesForRow(const FDFEnemyTableRow& Row);

	/** Stops the brain, movement, and AI controller input. */
	void DisableEnemyActions();

	FTimerHandle DeathDestroyTimer;

	UPROPERTY(Transient, DuplicateTransient)
	bool bAttributeDelegatesBound = false;

	UPROPERTY(Transient, DuplicateTransient)
	bool bHasDied = false;

	/** Filled in InitializeFromDataTable for loot. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|Enemy")
	TArray<FName> CachedLootTableRowNames;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|Enemy")
	float CachedExperienceReward = 0.f;

	/** Server-only: who last applied damage (effect causer / instigator). */
	TWeakObjectPtr<AActor> LastDamageAttacker;

	TObjectPtr<UBehaviorTree> CachedAIBehaviorTree = nullptr;
	FGenericTeamId TeamId;
};
