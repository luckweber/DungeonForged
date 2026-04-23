// Source/DungeonForged/Private/AI/UDFBTService_UpdateTarget.cpp

#include "AI/UDFBTService_UpdateTarget.h"
#include "AI/DFAIKeys.h"
#include "Characters/ADFEnemyBase.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "CollisionQueryParams.h"
#include "WorldCollision.h"
#include "GameFramework/PlayerController.h"
#include <limits>

UDFBTService_UpdateTarget::UDFBTService_UpdateTarget()
{
	NodeName = TEXT("DF UpdateTarget");
	Interval = 0.2f;
	RandomDeviation = 0.f;
	INIT_SERVICE_NODE_NOTIFY_FLAGS();
}

void UDFBTService_UpdateTarget::TickNode(
	UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/, const float /*DeltaSeconds*/)
{
	AAIController* const AI = OwnerComp.GetAIOwner();
	UBlackboardComponent* const BB = OwnerComp.GetBlackboardComponent();
	ADFEnemyBase* const Self = AI ? Cast<ADFEnemyBase>(AI->GetPawn()) : nullptr;
	UWorld* const W = OwnerComp.GetWorld();
	if (!IsValid(BB) || !IsValid(Self) || !W)
	{
		return;
	}
	const FVector Origin = Self->GetActorLocation();
	AActor* Best = nullptr;
	float BestD = std::numeric_limits<float>::max();
	// Simple: nearest player character (PIE player 0); extend to multi-overlap in shipping.
	if (APlayerController* const PC = UGameplayStatics::GetPlayerController(W, 0))
	{
		if (APawn* const P = PC->GetPawn())
		{
			const float D = FVector::Dist(Origin, P->GetActorLocation());
			if (D <= SearchRadius)
			{
				Best = P;
				BestD = D;
			}
		}
	}
	if (!IsValid(Best))
	{
		return;
	}
	bool bLineOk = true;
	if (bUseLineOfSight)
	{
		FCollisionQueryParams Pq(SCENE_QUERY_STAT(DF_BTSv_TargetLOS), true, Self);
		Pq.AddIgnoredActor(Best);
		FHitResult Hit;
		bLineOk = !W->LineTraceSingleByChannel(
			Hit, Origin + FVector(0, 0, 50.f), Best->GetActorLocation() + FVector(0, 0, 50.f),
			ECC_Visibility, Pq);
	}
	BB->SetValueAsObject(DFAIKeys::TargetActor, Best);
	BB->SetValueAsVector(DFAIKeys::TargetLocation, Best->GetActorLocation());
	BB->SetValueAsBool(DFAIKeys::bCanSeeTarget, bLineOk);
	const float R = FMath::Max(0.f, Self->AttackRange);
	BB->SetValueAsBool(DFAIKeys::bIsInAttackRange, BestD <= R);
	(void)BestD;
}
