// Source/DungeonForged/Private/AI/UDFBTDecorator_IsInRange.cpp

#include "AI/UDFBTDecorator_IsInRange.h"
#include "AI/DFAIKeys.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"

namespace
{
bool IsDFRangeTargetAlive(AActor* const Actor)
{
	if (!IsValid(Actor))
	{
		return false;
	}
	UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	if (!ASC)
	{
		return true;
	}
	if (FDFGameplayTags::State_Dead.IsValid() && ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Dead))
	{
		return false;
	}
	const FGameplayAttribute HealthAttribute = UDFAttributeSet::GetHealthAttribute();
	return !HealthAttribute.IsValid() || ASC->GetNumericAttribute(HealthAttribute) > 0.f;
}
} // namespace

UDFBTDecorator_IsInRange::UDFBTDecorator_IsInRange()
{
	NodeName = TEXT("DF IsInRange");
	INIT_DECORATOR_NODE_NOTIFY_FLAGS();
}

bool UDFBTDecorator_IsInRange::CalculateRawConditionValue(
	UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/) const
{
	AAIController* const AI = OwnerComp.GetAIOwner();
	UBlackboardComponent* const BB = OwnerComp.GetBlackboardComponent();
	if (!IsValid(AI) || !IsValid(BB))
	{
		return false;
	}
	APawn* const Self = AI->GetPawn();
	AActor* const T = Cast<AActor>(BB->GetValueAsObject(DFAIKeys::TargetActor));
	if (!IsValid(Self) || !IsDFRangeTargetAlive(T))
	{
		return false;
	}
	return FVector::Dist(Self->GetActorLocation(), T->GetActorLocation()) <= FMath::Max(0.f, Range);
}
