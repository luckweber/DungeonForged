// Source/DungeonForged/Private/AI/UDFBTTask_PlayTauntMontage.cpp

#include "AI/UDFBTTask_PlayTauntMontage.h"
#include "Characters/ADFEnemyBase.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

UDFBTTask_PlayTauntMontage::UDFBTTask_PlayTauntMontage()
{
	NodeName = TEXT("DF PlayTauntMontage");
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UDFBTTask_PlayTauntMontage::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	AAIController* const AI = OwnerComp.GetAIOwner();
	ADFEnemyBase* const E = AI ? Cast<ADFEnemyBase>(AI->GetPawn()) : nullptr;
	if (!IsValid(E) || E->TauntMontages.IsEmpty())
	{
		return EBTNodeResult::Failed;
	}
	const int32 I = FMath::RandRange(0, E->TauntMontages.Num() - 1);
	UAnimMontage* const M = E->TauntMontages[I];
	if (!M || !E->GetMesh())
	{
		return EBTNodeResult::Failed;
	}
	if (UAnimInstance* const Anim = E->GetMesh()->GetAnimInstance())
	{
		Anim->Montage_Play(M, 1.f);
	}
	return EBTNodeResult::Succeeded;
}
