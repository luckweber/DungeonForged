// Source/DungeonForged/Private/AI/UDFBTTask_BossMinionGuard.cpp
#include "AI/UDFBTTask_BossMinionGuard.h"
#include "AI/DFAIKeys.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"

namespace
{
struct FDFGuardMem
{
	bool bIssued = false;
};
} // namespace

UDFBTTask_BossMinionGuard::UDFBTTask_BossMinionGuard()
{
	NodeName = TEXT("DF BossMinionGuard");
	bNotifyTick = true;
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

uint16 UDFBTTask_BossMinionGuard::GetInstanceMemorySize() const
{
	return sizeof(FDFGuardMem);
}

EBTNodeResult::Type UDFBTTask_BossMinionGuard::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* const NodeMemory)
{
	auto* const M = new (NodeMemory) FDFGuardMem();
	UBlackboardComponent* const BB = OwnerComp.GetBlackboardComponent();
	AAIController* const AI = OwnerComp.GetAIOwner();
	APawn* const Self = AI ? AI->GetPawn() : nullptr;
	if (!IsValid(BB) || !IsValid(Self) || !BB->GetValueAsBool(DFAIKeys::bBossMinionGuarding))
	{
		return EBTNodeResult::Failed;
	}
	const FVector GuardPoint = BB->GetValueAsVector(DFAIKeys::TargetLocation);
	const EPathFollowingRequestResult::Type Result = AI->MoveToLocation(
		GuardPoint, AcceptanceRadius, true, true, true, true);
	if (Result == EPathFollowingRequestResult::Failed)
	{
		return EBTNodeResult::Failed;
	}
	if (Result == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		return EBTNodeResult::Succeeded;
	}
	M->bIssued = true;
	return EBTNodeResult::InProgress;
}

void UDFBTTask_BossMinionGuard::TickTask(
	UBehaviorTreeComponent& OwnerComp, uint8* const NodeMemory, const float /*DeltaSeconds*/)
{
	auto* const M = reinterpret_cast<FDFGuardMem*>(NodeMemory);
	UBlackboardComponent* const BB = OwnerComp.GetBlackboardComponent();
	AAIController* const AI = OwnerComp.GetAIOwner();
	if (!IsValid(BB) || !IsValid(AI) || !M->bIssued)
	{
		return;
	}
	if (!BB->GetValueAsBool(DFAIKeys::bBossMinionGuarding))
	{
		AI->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	if (UPathFollowingComponent* const Pfc = AI->GetPathFollowingComponent())
	{
		if (Pfc->GetStatus() == EPathFollowingStatus::Idle)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
	}
}

EBTNodeResult::Type UDFBTTask_BossMinionGuard::AbortTask(
	UBehaviorTreeComponent& OwnerComp, uint8* const NodeMemory)
{
	(void)NodeMemory;
	if (AAIController* const AI = OwnerComp.GetAIOwner())
	{
		AI->StopMovement();
	}
	return EBTNodeResult::Aborted;
}
