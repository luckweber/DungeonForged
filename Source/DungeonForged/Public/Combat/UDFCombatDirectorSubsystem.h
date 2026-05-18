// Source/DungeonForged/Public/Combat/UDFCombatDirectorSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Data/DFDataTableStructs.h"
#include "Subsystems/WorldSubsystem.h"
#include "UDFCombatDirectorSubsystem.generated.h"

class ADFEnemyBase;
class ADFPlayerCharacter;

/** Limits simultaneous enemy melee attackers and tracks nearby threat for music/UI. */
UCLASS()
class DUNGEONFORGED_API UDFCombatDirectorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterEnemy(ADFEnemyBase* Enemy);
	void UnregisterEnemy(ADFEnemyBase* Enemy);

	bool RequestAttackToken(ADFEnemyBase* Enemy);
	void ReleaseAttackToken(ADFEnemyBase* Enemy);

	UFUNCTION(BlueprintPure, Category = "DF|Combat|Director")
	int32 GetActiveAttackerCount() const;

	UFUNCTION(BlueprintPure, Category = "DF|Combat|Director")
	int32 GetRegisteredEnemyCount() const;

	/** Enemies within @c RangeCm of the local player pawn. */
	UFUNCTION(BlueprintPure, Category = "DF|Combat|Director")
	int32 GetEnemiesNearLocalPlayer(float RangeCm = 2000.f) const;

	UPROPERTY(EditAnywhere, Category = "DF|Combat|Director", meta = (ClampMin = "1"))
	int32 MaxAttackTokens = 2;

	UPROPERTY(EditAnywhere, Category = "DF|Combat|Director")
	int32 HighIntensityEnemyThreshold = 4;

	static int32 GetArchetypeAttackPriority(EDFEnemyArchetype Archetype);

private:
	int32 GetEnemyPriority(ADFEnemyBase* Enemy) const;
	void PruneInvalid();

	TArray<TWeakObjectPtr<ADFEnemyBase>> RegisteredEnemies;
	TArray<TWeakObjectPtr<ADFEnemyBase>> ActiveAttackers;
};
