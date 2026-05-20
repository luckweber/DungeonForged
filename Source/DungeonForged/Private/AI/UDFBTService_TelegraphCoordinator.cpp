// Source/DungeonForged/Private/AI/UDFBTService_TelegraphCoordinator.cpp
#include "AI/UDFBTService_TelegraphCoordinator.h"

#include "AI/DFAIKeys.h"
#include "AI/UDFAIAwarenessSubsystem.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"

UDFBTService_TelegraphCoordinator::UDFBTService_TelegraphCoordinator()
{
	NodeName = TEXT("DF TelegraphCoordinator");
	Interval = 0.3f;
	RandomDeviation = 0.05f;
	INIT_SERVICE_NODE_NOTIFY_FLAGS();
}

void UDFBTService_TelegraphCoordinator::TickNode(
	UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/, const float /*DeltaSeconds*/)
{
	UBlackboardComponent* const BB = OwnerComp.GetBlackboardComponent();
	AAIController* const AI = OwnerComp.GetAIOwner();
	APawn* const Pawn = AI ? AI->GetPawn() : nullptr;
	if (!IsValid(BB) || !IsValid(Pawn))
	{
		return;
	}
	UWorld* const World = OwnerComp.GetWorld();
	UDFAIAwarenessSubsystem* const Awareness = World ? World->GetSubsystem<UDFAIAwarenessSubsystem>() : nullptr;
	if (!Awareness)
	{
		BB->SetValueAsBool(DFAIKeys::bCanTelegraph, true);
		return;
	}
	const int32 LocalMax = FMath::Max(1, MaxConcurrentTelegraphs > 0 ? MaxConcurrentTelegraphs : Awareness->MaxConcurrentTelegraphs);
	const int32 Count = Awareness->GetTelegraphingCountWithin(Pawn->GetActorLocation(), CoordinationRadiusCm);
	BB->SetValueAsBool(DFAIKeys::bCanTelegraph, Count < LocalMax);
}
