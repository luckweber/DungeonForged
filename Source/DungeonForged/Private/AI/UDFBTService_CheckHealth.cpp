// Source/DungeonForged/Private/AI/UDFBTService_CheckHealth.cpp

#include "AI/UDFBTService_CheckHealth.h"
#include "AI/DFAIKeys.h" // EADFAICombatState
#include "Characters/ADFEnemyBase.h"
#include "GAS/UDFAttributeSet.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UDFBTService_CheckHealth::UDFBTService_CheckHealth()
{
	NodeName = TEXT("DF CheckHealth");
	Interval = 0.5f;
	RandomDeviation = 0.f;
	INIT_SERVICE_NODE_NOTIFY_FLAGS();
}

void UDFBTService_CheckHealth::TickNode(
	UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/, const float /*DeltaSeconds*/)
{
	AAIController* const AI = OwnerComp.GetAIOwner();
	UBlackboardComponent* const BB = OwnerComp.GetBlackboardComponent();
	ADFEnemyBase* const E = AI ? Cast<ADFEnemyBase>(AI->GetPawn()) : nullptr;
	if (!IsValid(BB) || !IsValid(E) || !E->GetAbilitySystemComponent())
	{
		return;
	}
	const UDFAttributeSet* const AS = E->GetAbilitySystemComponent()->GetSet<UDFAttributeSet>();
	if (!AS)
	{
		return;
	}
	const float Hp = AS->GetHealth();
	const float Mx = FMath::Max(1.f, AS->GetMaxHealth());
	if (Hp <= 0.f)
	{
		BB->SetValueAsBool(DFAIKeys::bIsDead, true);
		return;
	}
	if (Mx > KINDA_SMALL_NUMBER && (Hp / Mx) < FleeHealthFraction)
	{
		BB->SetValueAsEnum(DFAIKeys::CombatState, static_cast<uint8>(EADFAICombatState::Flee));
	}
}
