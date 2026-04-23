// Source/DungeonForged/Public/AI/DFAIKeys.h
#pragma once

#include "CoreMinimal.h"
#include "DFAIKeys.generated.h"

/**
 * Project convention: C++ name BB_*; blackboard string keys are short to match
 * a typical UBlackboardData asset (e.g. "TargetActor" not "BB_TargetActor").
 * Match these names when you author BT_EnemyBase in the editor.
 */
namespace DFAIKeys
{
	inline const FName TargetActor = TEXT("TargetActor");
	inline const FName TargetLocation = TEXT("TargetLocation");
	inline const FName bCanSeeTarget = TEXT("bCanSeeTarget");
	inline const FName bIsInAttackRange = TEXT("bIsInAttackRange");
	inline const FName bIsDead = TEXT("bIsDead");
	inline const FName PatrolIndex = TEXT("PatrolIndex");
	inline const FName CombatState = TEXT("CombatState");
}

UENUM(BlueprintType)
enum class EADFAICombatState : uint8
{
	Idle   UMETA(DisplayName = "Idle"),
	Patrol UMETA(DisplayName = "Patrol"),
	Chase  UMETA(DisplayName = "Chase"),
	Attack UMETA(DisplayName = "Attack"),
	Flee   UMETA(DisplayName = "Flee")
};

/**
 * Optional designer reference — BT layout for BT_EnemyBase.
 *
 * Root -> Selector
 *   - [Blackboard: bIsDead == true]  ->  UDFBTTask_Die
 *   - Sequence (Chase + Attack)
 *         [Decorator: Blackboard: TargetActor IsSet]  (or custom HasTarget)
 *     - Service: UDFBTService_UpdateTarget (0.2s)
 *     - Service: UDFBTService_CheckHealth (0.5s)
 *     - Selector
 *         - [UDFBTDecorator_IsInRange: MeleeRange]  -> UDFBTTask_MeleeAttack
 *         - [UDFBTDecorator_IsInRange: RangedRange]  -> UDFBTTask_RangedAttack
 *         - BTTask_MoveTo (TargetActor) or MoveTo (TargetActor)
 *     - (Optional) When CombatState==Flee: UDFBTTask_FleeFromPlayer
 *   - Sequence (Patrol)
 *     - UDFBTTask_FindPatrolPoint
 *     - BTTask_MoveTo (TargetLocation)
 */
