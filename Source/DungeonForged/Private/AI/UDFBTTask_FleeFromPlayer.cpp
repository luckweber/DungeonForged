// Source/DungeonForged/Private/AI/UDFBTTask_FleeFromPlayer.cpp

#include "AI/UDFBTTask_FleeFromPlayer.h"
#include "AIController.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

namespace
{
struct FDFleeMem
{
	bool bIssued = false;
	double TimeEnd = 0.0;
};
} // namespace

UDFBTTask_FleeFromPlayer::UDFBTTask_FleeFromPlayer()
{
	NodeName = TEXT("DF FleeFromPlayer");
	bNotifyTick = true;
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

uint16 UDFBTTask_FleeFromPlayer::GetInstanceMemorySize() const
{
	return sizeof(FDFleeMem);
}

EBTNodeResult::Type UDFBTTask_FleeFromPlayer::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* const NodeMemory)
{
	auto* const M = new (NodeMemory) FDFleeMem();
	APawn* const Self = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
	if (!IsValid(Self))
	{
		return EBTNodeResult::Failed;
	}
	APlayerController* const PC = UGameplayStatics::GetPlayerController(Self, 0);
	APawn* const PlayerPawn = PC ? PC->GetPawn() : nullptr;
	if (!IsValid(PlayerPawn))
	{
		return EBTNodeResult::Failed;
	}
	const FVector FromPlayer = (Self->GetActorLocation() - PlayerPawn->GetActorLocation()).GetSafeNormal2D();
	FVector FleePoint = Self->GetActorLocation() + FromPlayer * FleeSampleDistance;
	if (UNavigationSystemV1* const Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(OwnerComp.GetWorld()))
	{
		FNavLocation NavLoc;
		if (Nav->ProjectPointToNavigation(FleePoint, NavLoc, FVector(1000.f, 1000.f, 500.f)))
		{
			FleePoint = NavLoc.Location;
		}
	}
	AAIController* const AI = OwnerComp.GetAIOwner();
	if (!AI)
	{
		return EBTNodeResult::Failed;
	}
	const EPathFollowingRequestResult::Type R = AI->MoveToLocation(
		FleePoint, AcceptanceRadius, true, true, true, true);
	if (R == EPathFollowingRequestResult::Failed)
	{
		return EBTNodeResult::Failed;
	}
	if (R == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		return EBTNodeResult::Succeeded;
	}
	M->bIssued = true;
	if (UWorld* const W = OwnerComp.GetWorld())
	{
		M->TimeEnd = W->GetTimeSeconds() + 12.0; // cap wait
	}
	// RequestSuccessful: path follow in progress.
	return EBTNodeResult::InProgress;
}

void UDFBTTask_FleeFromPlayer::TickTask(
	UBehaviorTreeComponent& OwnerComp, uint8* const NodeMemory, const float /*DeltaSeconds*/)
{
	auto* const M = reinterpret_cast<FDFleeMem*>(NodeMemory);
	if (!M->bIssued)
	{
		return;
	}
	AAIController* const AI = OwnerComp.GetAIOwner();
	if (!AI)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	if (AI->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}
	if (UWorld* const W = OwnerComp.GetWorld())
	{
		if (M->TimeEnd > 0.0 && W->GetTimeSeconds() > M->TimeEnd)
		{
			AI->StopMovement();
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
	}
}

EBTNodeResult::Type UDFBTTask_FleeFromPlayer::AbortTask(
	UBehaviorTreeComponent& OwnerComp, uint8* const /*NodeMemory*/)
{
	if (AAIController* const AI = OwnerComp.GetAIOwner())
	{
		AI->StopMovement();
	}
	return EBTNodeResult::Aborted;
}
