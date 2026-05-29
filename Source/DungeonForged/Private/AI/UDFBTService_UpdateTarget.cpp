// Source/DungeonForged/Private/AI/UDFBTService_UpdateTarget.cpp

#include "AI/UDFBTService_UpdateTarget.h"
#include "AI/ADFAIController.h"
#include "AI/DFAIKeys.h"
#include "AI/UDFAILibrary.h"
#include "AI/UDFEnemyArchetypeLibrary.h"
#include "Characters/ADFEnemyBase.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

UDFBTService_UpdateTarget::UDFBTService_UpdateTarget()
{
	NodeName = TEXT("DF UpdateTarget");
	Interval = 0.2f;
	RandomDeviation = 0.f;
	INIT_SERVICE_NODE_NOTIFY_FLAGS();
}

void UDFBTService_UpdateTarget::TickNode(
	UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/, const float /*DeltaSeconds*/)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("DungeonForged.AIUpdateTarget"));
	AAIController* const AI = OwnerComp.GetAIOwner();
	UBlackboardComponent* const BB = OwnerComp.GetBlackboardComponent();
	ADFEnemyBase* const Self = AI ? Cast<ADFEnemyBase>(AI->GetPawn()) : nullptr;
	if (!IsValid(BB) || !IsValid(Self))
	{
		return;
	}
	const FVector Origin = Self->GetActorLocation();
	AActor* const CurrentTarget = Cast<AActor>(BB->GetValueAsObject(DFAIKeys::TargetActor));
	AActor* const Best = UDFAILibrary::FindBestHostilePlayerTarget(
		OwnerComp.GetWorld(),
		Self,
		Origin,
		SearchRadius,
		CurrentTarget,
		bUseLineOfSight);
	if (!IsValid(Best))
	{
		if (IsValid(CurrentTarget))
		{
			const FVector LastKnown = CurrentTarget->GetActorLocation();
			BB->SetValueAsVector(DFAIKeys::LastKnownTargetLocation, LastKnown);
			BB->SetValueAsVector(DFAIKeys::TargetLocation, LastKnown);
			BB->SetValueAsBool(DFAIKeys::bHasLastKnownTarget, true);
		}
		BB->ClearValue(DFAIKeys::TargetActor);
		BB->SetValueAsBool(DFAIKeys::bCanSeeTarget, false);
		BB->SetValueAsBool(DFAIKeys::bIsInAttackRange, false);
		if (BB->GetValueAsBool(DFAIKeys::bHasLastKnownTarget))
		{
			if (ADFAIController* const DFAI = Cast<ADFAIController>(AI))
			{
				const EADFAICombatState State = static_cast<EADFAICombatState>(BB->GetValueAsEnum(DFAIKeys::CombatState));
				if (State != EADFAICombatState::Flee && State != EADFAICombatState::Recover)
				{
					DFAI->SetCombatState(EADFAICombatState::Investigate);
				}
			}
		}
		return;
	}
	const float BestD = FVector::Dist(Origin, Best->GetActorLocation());
	bool bLineOk = true;
	if (bUseLineOfSight)
	{
		FCollisionQueryParams Pq(SCENE_QUERY_STAT(DF_BTSv_TargetLOS), true, Self);
		Pq.AddIgnoredActor(Best);
		FHitResult Hit;
		bLineOk = !OwnerComp.GetWorld()->LineTraceSingleByChannel(
			Hit,
			Origin + FVector(0, 0, 50.f),
			Best->GetActorLocation() + FVector(0, 0, 50.f),
			ECC_Visibility,
			Pq);
	}
	const FVector TargetLoc = Best->GetActorLocation();
	BB->SetValueAsObject(DFAIKeys::TargetActor, Best);
	BB->SetValueAsVector(DFAIKeys::TargetLocation, TargetLoc);
	BB->SetValueAsVector(DFAIKeys::LastKnownTargetLocation, TargetLoc);
	BB->SetValueAsBool(DFAIKeys::bHasLastKnownTarget, true);
	BB->SetValueAsBool(DFAIKeys::bCanSeeTarget, bLineOk);
	const float PreferredRange = UDFEnemyArchetypeLibrary::GetPreferredInRangeDistance(
		Self->GetEnemyArchetype(), Self->MeleeRange, Self->RangedRange);
	BB->SetValueAsBool(DFAIKeys::bIsInAttackRange, BestD <= PreferredRange);
	BB->SetValueAsBool(
		DFAIKeys::bPrefersRangedCombat, UDFEnemyArchetypeLibrary::PrefersRangedCombat(Self->GetEnemyArchetype()));
	if (ADFAIController* const DFAI = Cast<ADFAIController>(AI))
	{
		const EADFAICombatState State = static_cast<EADFAICombatState>(BB->GetValueAsEnum(DFAIKeys::CombatState));
		if (bLineOk)
		{
			if (State == EADFAICombatState::Investigate || State == EADFAICombatState::Patrol)
			{
				DFAI->SetCombatState(EADFAICombatState::Chase);
			}
		}
		else if (State != EADFAICombatState::Flee && State != EADFAICombatState::Recover)
		{
			DFAI->SetCombatState(EADFAICombatState::Investigate);
		}
	}
}
