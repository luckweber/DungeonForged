// Source/DungeonForged/Private/AI/UDFBTDecorator_IsInRange.cpp

#include "AI/UDFBTDecorator_IsInRange.h"
#include "AI/DFAIKeys.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"

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
	if (!IsValid(Self) || !IsValid(T))
	{
		return false;
	}
	return FVector::Dist(Self->GetActorLocation(), T->GetActorLocation()) <= FMath::Max(0.f, Range);
}
