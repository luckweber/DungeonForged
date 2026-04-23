// Source/DungeonForged/Private/AI/UDFBTTask_RangedAttack.cpp

#include "AI/UDFBTTask_RangedAttack.h"
#include "AI/DFAIKeys.h"
#include "GAS/DFGameplayTags.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"

UDFBTTask_RangedAttack::UDFBTTask_RangedAttack()
{
	NodeName = TEXT("DF RangedAttack");
	RequiredTag = FDFGameplayTags::Ability_Attack_Ranged;
}

EBTNodeResult::Type UDFBTTask_RangedAttack::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* const AI = OwnerComp.GetAIOwner())
	{
		if (UBlackboardComponent* const BB = OwnerComp.GetBlackboardComponent())
		{
			if (AActor* const T = Cast<AActor>(BB->GetValueAsObject(DFAIKeys::TargetActor)))
			{
				AI->SetFocus(T, EAIFocusPriority::Gameplay);
			}
		}
	}
	return UDFBTTask_MeleeAttack::ExecuteTask(OwnerComp, NodeMemory);
}
