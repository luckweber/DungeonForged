// Source/DungeonForged/Private/AI/UDFBTTask_InvestigateLastKnown.cpp
#include "AI/UDFBTTask_InvestigateLastKnown.h"

#include "AI/ADFAIController.h"
#include "AI/DFAIKeys.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"

namespace
{
struct FDFInvestigateMem
{
	bool bIssued = false;
	double TimeEnd = 0.0;
};
} // namespace

UDFBTTask_InvestigateLastKnown::UDFBTTask_InvestigateLastKnown()
{
	NodeName = TEXT("DF InvestigateLastKnown");
	bNotifyTick = true;
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

uint16 UDFBTTask_InvestigateLastKnown::GetInstanceMemorySize() const
{
	return sizeof(FDFInvestigateMem);
}

EBTNodeResult::Type UDFBTTask_InvestigateLastKnown::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* const NodeMemory)
{
	auto* const Mem = new (NodeMemory) FDFInvestigateMem();
	UBlackboardComponent* const BB = OwnerComp.GetBlackboardComponent();
	AAIController* const AI = OwnerComp.GetAIOwner();
	APawn* const Self = AI ? AI->GetPawn() : nullptr;
	if (!IsValid(BB) || !IsValid(Self) || !BB->GetValueAsBool(DFAIKeys::bHasLastKnownTarget))
	{
		return EBTNodeResult::Failed;
	}
	const FVector InvestigatePoint = BB->GetValueAsVector(DFAIKeys::LastKnownTargetLocation);
	BB->SetValueAsVector(DFAIKeys::TargetLocation, InvestigatePoint);
	if (ADFAIController* const DFAI = Cast<ADFAIController>(AI))
	{
		DFAI->SetCombatState(EADFAICombatState::Investigate);
	}
	if (UNavigationSystemV1* const Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(OwnerComp.GetWorld()))
	{
		FNavLocation NavLoc;
		if (Nav->ProjectPointToNavigation(InvestigatePoint, NavLoc, FVector(400.f, 400.f, 300.f)))
		{
			BB->SetValueAsVector(DFAIKeys::TargetLocation, NavLoc.Location);
		}
	}
	if (!AI)
	{
		return EBTNodeResult::Failed;
	}
	const EPathFollowingRequestResult::Type MoveResult = AI->MoveToLocation(
		BB->GetValueAsVector(DFAIKeys::TargetLocation),
		AcceptanceRadius,
		true,
		true,
		false,
		true);
	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		if (UBlackboardComponent* const ClearBB = OwnerComp.GetBlackboardComponent())
		{
			ClearBB->SetValueAsBool(DFAIKeys::bHasLastKnownTarget, false);
		}
		return EBTNodeResult::Failed;
	}
	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		if (UBlackboardComponent* const ClearBB = OwnerComp.GetBlackboardComponent())
		{
			ClearBB->SetValueAsBool(DFAIKeys::bHasLastKnownTarget, false);
		}
		if (ADFAIController* const DFAI = Cast<ADFAIController>(AI))
		{
			DFAI->SetCombatState(EADFAICombatState::Patrol);
		}
		return EBTNodeResult::Succeeded;
	}
	Mem->bIssued = true;
	if (UWorld* const World = OwnerComp.GetWorld())
	{
		Mem->TimeEnd = World->GetTimeSeconds() + InvestigateTimeoutSeconds;
	}
	return EBTNodeResult::InProgress;
}

void UDFBTTask_InvestigateLastKnown::TickTask(
	UBehaviorTreeComponent& OwnerComp, uint8* const NodeMemory, const float /*DeltaSeconds*/)
{
	auto* const Mem = reinterpret_cast<FDFInvestigateMem*>(NodeMemory);
	if (!Mem->bIssued)
	{
		return;
	}
	AAIController* const AI = OwnerComp.GetAIOwner();
	if (!AI)
	{
		FinishInvestigation(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	if (AI->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		FinishInvestigation(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}
	if (UWorld* const World = OwnerComp.GetWorld())
	{
		if (Mem->TimeEnd > 0.0 && World->GetTimeSeconds() > Mem->TimeEnd)
		{
			AI->StopMovement();
			FinishInvestigation(OwnerComp, EBTNodeResult::Succeeded);
		}
	}
}

EBTNodeResult::Type UDFBTTask_InvestigateLastKnown::AbortTask(
	UBehaviorTreeComponent& OwnerComp, uint8* const /*NodeMemory*/)
{
	if (AAIController* const AI = OwnerComp.GetAIOwner())
	{
		AI->StopMovement();
	}
	return EBTNodeResult::Aborted;
}

void UDFBTTask_InvestigateLastKnown::FinishInvestigation(
	UBehaviorTreeComponent& OwnerComp, const EBTNodeResult::Type Result)
{
	if (UBlackboardComponent* const BB = OwnerComp.GetBlackboardComponent())
	{
		BB->SetValueAsBool(DFAIKeys::bHasLastKnownTarget, false);
		BB->ClearValue(DFAIKeys::LastKnownTargetLocation);
	}
	if (ADFAIController* const DFAI = Cast<ADFAIController>(OwnerComp.GetAIOwner()))
	{
		DFAI->SetCombatState(EADFAICombatState::Patrol);
	}
	FinishLatentTask(OwnerComp, Result);
}
