// Source/DungeonForged/Public/AI/UDFEnemyArchetypeLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Data/DFDataTableStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UDFEnemyArchetypeLibrary.generated.h"

/** C++ tuning for @c EDFEnemyArchetype — flee, spacing, token priority, movement. */
UCLASS()
class DUNGEONFORGED_API UDFEnemyArchetypeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "DF|AI|Archetype")
	static bool PrefersRangedCombat(EDFEnemyArchetype Archetype);

	/** HP ratio below which @c EADFAICombatState::Flee is entered. */
	UFUNCTION(BlueprintPure, Category = "DF|AI|Archetype")
	static float GetFleeEnterHealthFraction(EDFEnemyArchetype Archetype);

	/** Hysteresis: resume Chase when HP rises above this while fleeing. */
	UFUNCTION(BlueprintPure, Category = "DF|AI|Archetype")
	static float GetFleeReturnHealthFraction(EDFEnemyArchetype Archetype);

	/** Range used by UpdateTarget for @c bIsInAttackRange (cm). */
	UFUNCTION(BlueprintPure, Category = "DF|AI|Archetype")
	static float GetPreferredInRangeDistance(EDFEnemyArchetype Archetype, float MeleeRange, float RangedRange);

	/** Melee attack token priority for @c UDFCombatDirectorSubsystem. */
	UFUNCTION(BlueprintPure, Category = "DF|AI|Archetype")
	static int32 GetMeleeAttackTokenPriority(EDFEnemyArchetype Archetype);

	/** Ranged cast slot priority (Caster/Sniper/Healer). */
	UFUNCTION(BlueprintPure, Category = "DF|AI|Archetype")
	static int32 GetRangedCastTokenPriority(EDFEnemyArchetype Archetype);

	/** Telegraph windup slot priority (higher may preempt lower). */
	UFUNCTION(BlueprintPure, Category = "DF|AI|Archetype")
	static int32 GetTelegraphPriority(EDFEnemyArchetype Archetype, EEnemyTier Tier);

	/** Applied to row @c MaxWalkSpeed when > 0. */
	UFUNCTION(BlueprintPure, Category = "DF|AI|Archetype")
	static float GetMovementSpeedScale(EDFEnemyArchetype Archetype);

	/** Flee task sample distance (cm) away from the player. */
	UFUNCTION(BlueprintPure, Category = "DF|AI|Archetype")
	static float GetFleeSampleDistance(EDFEnemyArchetype Archetype);

	/** When a data row leaves @c GrantedAbilities empty, grant this attack tag. */
	UFUNCTION(BlueprintPure, Category = "DF|AI|Archetype")
	static FGameplayTagContainer GetDefaultGrantedAbilityTags(EDFEnemyArchetype Archetype);

	/** Optional row fallbacks when designer leaves ranges at 0. */
	static void ApplyArchetypeRangeDefaults(EDFEnemyArchetype Archetype, float& InOutMeleeRange, float& InOutRangedRange, float& InOutAttackRange);
};
