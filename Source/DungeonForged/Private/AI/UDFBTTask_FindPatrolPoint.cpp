// Source/DungeonForged/Private/AI/UDFBTTask_FindPatrolPoint.cpp

#include "AI/UDFBTTask_FindPatrolPoint.h"
#include "AI/DFAIKeys.h"
#include "Characters/ADFEnemyBase.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"

UDFBTTask_FindPatrolPoint::UDFBTTask_FindPatrolPoint()
{
	NodeName = TEXT("DF FindPatrolPoint");
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UDFBTTask_FindPatrolPoint::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	AAIController* const AI = OwnerComp.GetAIOwner();
	UBlackboardComponent* const BB = OwnerComp.GetBlackboardComponent();
	ADFEnemyBase* const E = AI ? Cast<ADFEnemyBase>(AI->GetPawn()) : nullptr;
	if (!IsValid(BB) || !IsValid(E) || E->PatrolPoints.IsEmpty())
	{
		return EBTNodeResult::Failed;
	}
	int32 Index = FMath::Max(0, BB->GetValueAsInt(DFAIKeys::PatrolIndex));
	const int32 N = E->PatrolPoints.Num();
	const FVector Next = E->PatrolPoints[Index % N];
	const int32 NextIndex = (Index + 1) % N;
	BB->SetValueAsInt(DFAIKeys::PatrolIndex, NextIndex);
	BB->SetValueAsVector(DFAIKeys::TargetLocation, Next);
	return EBTNodeResult::Succeeded;
}
