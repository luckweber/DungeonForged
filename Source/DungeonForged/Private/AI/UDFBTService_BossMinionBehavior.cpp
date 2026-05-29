// Source/DungeonForged/Private/AI/UDFBTService_BossMinionBehavior.cpp
#include "AI/UDFBTService_BossMinionBehavior.h"
#include "AI/DFAIKeys.h"
#include "Boss/ADFBossBase.h"
#include "Boss/UDFBossMinionComponent.h"
#include "Characters/ADFEnemyBase.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UDFBTService_BossMinionBehavior::UDFBTService_BossMinionBehavior()
{
	NodeName = TEXT("DF BossMinionBehavior");
	Interval = 0.25f;
	RandomDeviation = 0.05f;
	INIT_SERVICE_NODE_NOTIFY_FLAGS();
}

void UDFBTService_BossMinionBehavior::TickNode(
	UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/, const float /*DeltaSeconds*/)
{
	UBlackboardComponent* const BB = OwnerComp.GetBlackboardComponent();
	AAIController* const AI = OwnerComp.GetAIOwner();
	ADFEnemyBase* const Self = AI ? Cast<ADFEnemyBase>(AI->GetPawn()) : nullptr;
	if (!IsValid(BB) || !IsValid(Self))
	{
		return;
	}
	UDFBossMinionComponent* const Minion = Self->FindComponentByClass<UDFBossMinionComponent>();
	if (!Minion)
	{
		BB->SetValueAsBool(DFAIKeys::bBossMinionGuarding, false);
		return;
	}
	if (Minion->GetMinionRole() == EDFBossMinionRole::Exploder)
	{
		BB->SetValueAsBool(DFAIKeys::bBossMinionGuarding, false);
		Minion->TryDetonateFromProximity();
		return;
	}
	FVector GuardLoc = FVector::ZeroVector;
	const bool bGuarding = Minion->ComputeGuardLocation(GuardLoc);
	BB->SetValueAsBool(DFAIKeys::bBossMinionGuarding, bGuarding);
	if (bGuarding)
	{
		BB->SetValueAsVector(DFAIKeys::TargetLocation, GuardLoc);
		if (ADFBossBase* const Boss = Minion->GetOwningBoss())
		{
			BB->SetValueAsObject(DFAIKeys::BossOwnerActor, Boss);
		}
	}
}
