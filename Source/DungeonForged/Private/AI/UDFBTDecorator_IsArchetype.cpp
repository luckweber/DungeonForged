// Source/DungeonForged/Private/AI/UDFBTDecorator_IsArchetype.cpp
#include "AI/UDFBTDecorator_IsArchetype.h"

#include "Characters/ADFEnemyBase.h"
#include "AIController.h"

UDFBTDecorator_IsArchetype::UDFBTDecorator_IsArchetype()
{
	NodeName = TEXT("DF IsArchetype");
	INIT_DECORATOR_NODE_NOTIFY_FLAGS();
}

bool UDFBTDecorator_IsArchetype::CalculateRawConditionValue(
	UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/) const
{
	AAIController* const AI = OwnerComp.GetAIOwner();
	const ADFEnemyBase* const Enemy = AI ? Cast<ADFEnemyBase>(AI->GetPawn()) : nullptr;
	return IsValid(Enemy) && Enemy->GetEnemyArchetype() == Archetype;
}
