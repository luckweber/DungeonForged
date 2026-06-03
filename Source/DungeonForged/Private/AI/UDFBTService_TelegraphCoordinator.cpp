// Source/DungeonForged/Private/AI/UDFBTService_TelegraphCoordinator.cpp
#include "AI/UDFBTService_TelegraphCoordinator.h"

#include "AI/DFAIKeys.h"
#include "AI/UDFAIAwarenessSubsystem.h"
#include "Characters/ADFEnemyBase.h"
#include "Combat/UDFCombatDirectorSubsystem.h"
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
	UDFCombatDirectorSubsystem* const Director = World ? World->GetSubsystem<UDFCombatDirectorSubsystem>() : nullptr;
	ADFEnemyBase* const Enemy = Cast<ADFEnemyBase>(Pawn);
	bool bAllowed = true;
	if (Director && Enemy)
	{
		bAllowed = Director->CanEnemyTelegraph(Enemy);
	}
	else
	{
		UDFAIAwarenessSubsystem* const Awareness = World ? World->GetSubsystem<UDFAIAwarenessSubsystem>() : nullptr;
		if (Awareness)
		{
			const int32 LocalMax = FMath::Max(1, MaxConcurrentTelegraphs > 0 ? MaxConcurrentTelegraphs : Awareness->MaxConcurrentTelegraphs);
			const int32 Count = Awareness->GetTelegraphingCountWithin(Pawn->GetActorLocation(), CoordinationRadiusCm);
			bAllowed = Count < LocalMax;
		}
	}
	BB->SetValueAsBool(DFAIKeys::bCanTelegraph, bAllowed);
}
