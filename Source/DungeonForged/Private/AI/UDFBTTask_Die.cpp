// Source/DungeonForged/Private/AI/UDFBTTask_Die.cpp

#include "AI/UDFBTTask_Die.h"
#include "AI/DFAIKeys.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"

UDFBTTask_Die::UDFBTTask_Die()
{
	NodeName = TEXT("DF Die");
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UDFBTTask_Die::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	if (UBlackboardComponent* const BB = OwnerComp.GetBlackboardComponent())
	{
		BB->SetValueAsBool(DFAIKeys::bIsDead, true);
	}
	if (AAIController* const AI = OwnerComp.GetAIOwner())
	{
		AI->StopMovement();
		AI->ClearFocus(EAIFocusPriority::Gameplay);
	}
	return EBTNodeResult::Succeeded;
}
