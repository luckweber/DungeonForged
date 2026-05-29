// Source/DungeonForged/Private/AI/UDFBTDecorator_IsCombatState.cpp
#include "AI/UDFBTDecorator_IsCombatState.h"

#include "AI/DFAIKeys.h"
#include "BehaviorTree/BlackboardComponent.h"

UDFBTDecorator_IsCombatState::UDFBTDecorator_IsCombatState()
{
	NodeName = TEXT("DF IsCombatState");
	INIT_DECORATOR_NODE_NOTIFY_FLAGS();
}

bool UDFBTDecorator_IsCombatState::CalculateRawConditionValue(
	UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/) const
{
	const UBlackboardComponent* const BB = OwnerComp.GetBlackboardComponent();
	if (!IsValid(BB))
	{
		return false;
	}
	return static_cast<EADFAICombatState>(BB->GetValueAsEnum(DFAIKeys::CombatState)) == RequiredState;
}
