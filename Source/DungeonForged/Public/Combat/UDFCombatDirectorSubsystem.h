// Source/DungeonForged/Public/Combat/UDFCombatDirectorSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Data/DFDataTableStructs.h"
#include "Subsystems/WorldSubsystem.h"
#include "UDFCombatDirectorSubsystem.generated.h"

class ADFEnemyBase;
class ADFPlayerCharacter;

/** Limits simultaneous enemy attackers, ranged casts, telegraphs, and boss add budget. */
UCLASS()
class DUNGEONFORGED_API UDFCombatDirectorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterEnemy(ADFEnemyBase* Enemy);
	void UnregisterEnemy(ADFEnemyBase* Enemy);

	bool RequestAttackToken(ADFEnemyBase* Enemy);
	void ReleaseAttackToken(ADFEnemyBase* Enemy);

	bool RequestRangedCastToken(ADFEnemyBase* Enemy);
	void ReleaseRangedCastToken(ADFEnemyBase* Enemy);

	/** Returns whether @c Enemy may start a telegraph (may preempt lower priority). */
	bool CanEnemyTelegraph(ADFEnemyBase* Enemy) const;
	bool RequestTelegraphSlot(ADFEnemyBase* Enemy);
	void ReleaseTelegraphSlot(ADFEnemyBase* Enemy);

	bool CanSpawnBossMinion(const int32 Count = 1) const;
	void RegisterBossMinion(const int32 Count = 1);
	void UnregisterBossMinion(const int32 Count = 1);

	UFUNCTION(BlueprintPure, Category = "DF|Combat|Director")
	int32 GetActiveAttackerCount() const;

	UFUNCTION(BlueprintPure, Category = "DF|Combat|Director")
	int32 GetActiveRangedCasterCount() const;

	UFUNCTION(BlueprintPure, Category = "DF|Combat|Director")
	int32 GetActiveTelegraphCount() const;

	UFUNCTION(BlueprintPure, Category = "DF|Combat|Director")
	int32 GetActiveBossMinionCount() const;

	UFUNCTION(BlueprintPure, Category = "DF|Combat|Director")
	int32 GetRegisteredEnemyCount() const;

	/** Enemies within @c RangeCm of any player pawn. */
	UFUNCTION(BlueprintPure, Category = "DF|Combat|Director")
	int32 GetEnemiesNearLocalPlayer(float RangeCm = 2000.f) const;

	UPROPERTY(EditAnywhere, Category = "DF|Combat|Director", meta = (ClampMin = "1"))
	int32 MaxAttackTokens = 2;

	UPROPERTY(EditAnywhere, Category = "DF|Combat|Director", meta = (ClampMin = "0"))
	int32 MaxRangedCastTokens = 2;

	UPROPERTY(EditAnywhere, Category = "DF|Combat|Director", meta = (ClampMin = "1"))
	int32 MaxConcurrentTelegraphs = 2;

	UPROPERTY(EditAnywhere, Category = "DF|Combat|Director", meta = (ClampMin = "0"))
	int32 MaxBossMinionBudget = 12;

	UPROPERTY(EditAnywhere, Category = "DF|Combat|Director")
	int32 HighIntensityEnemyThreshold = 4;

	static int32 GetArchetypeAttackPriority(EDFEnemyArchetype Archetype);

	void PropagatePackAlert(ADFEnemyBase* SourceEnemy, AActor* TargetPlayer, FVector AlertLocation, float RadiusCm);

	UPROPERTY(EditAnywhere, Category = "DF|Combat|Director", meta = (ClampMin = "100.0"))
	float PackAlertRadiusCm = 1800.f;

private:
	int32 GetMeleePriority(ADFEnemyBase* Enemy) const;
	int32 GetRangedPriority(ADFEnemyBase* Enemy) const;
	int32 GetTelegraphPriority(ADFEnemyBase* Enemy) const;
	bool RequestPrioritySlotMutable(
		TArray<TWeakObjectPtr<ADFEnemyBase>>& Active,
		ADFEnemyBase* Enemy,
		int32 MaxSlots,
		int32 Priority,
		TFunctionRef<int32(ADFEnemyBase*)> PriorityFn) const;
	void ReleaseFromSlot(TArray<TWeakObjectPtr<ADFEnemyBase>>& Active, ADFEnemyBase* Enemy);
	void PruneInvalid();

	TArray<TWeakObjectPtr<ADFEnemyBase>> RegisteredEnemies;
	TArray<TWeakObjectPtr<ADFEnemyBase>> ActiveAttackers;
	TArray<TWeakObjectPtr<ADFEnemyBase>> ActiveRangedCasters;
	TArray<TWeakObjectPtr<ADFEnemyBase>> ActiveTelegraphers;
	int32 ActiveBossMinionCount = 0;
};
