// Source/DungeonForged/Private/AI/UDFBTDecorator_HasGASTag.cpp

#include "AI/UDFBTDecorator_HasGASTag.h"
#include "Characters/ADFEnemyBase.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"

UDFBTDecorator_HasGASTag::UDFBTDecorator_HasGASTag()
{
	NodeName = TEXT("DF HasGASTag");
	INIT_DECORATOR_NODE_NOTIFY_FLAGS();
}

bool UDFBTDecorator_HasGASTag::CalculateRawConditionValue(
	UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/) const
{
	if (!RequiredTag.IsValid())
	{
		return false;
	}
	AAIController* const AI = OwnerComp.GetAIOwner();
	const ADFEnemyBase* const E = AI ? Cast<ADFEnemyBase>(AI->GetPawn()) : nullptr;
	if (!IsValid(E) || !E->GetAbilitySystemComponent())
	{
		return false;
	}
	return E->GetAbilitySystemComponent()->HasMatchingGameplayTag(RequiredTag);
}
