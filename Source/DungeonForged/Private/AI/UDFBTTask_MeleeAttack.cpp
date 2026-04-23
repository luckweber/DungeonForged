// Source/DungeonForged/Private/AI/UDFBTTask_MeleeAttack.cpp

#include "AI/UDFBTTask_MeleeAttack.h"
#include "Characters/ADFEnemyBase.h"
#include "GAS/DFGameplayTags.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "HAL/UnrealMemory.h"
#include <new>

namespace
{
struct FDFMeleeMem
{
	FDelegateHandle Handle;
	TWeakObjectPtr<UAbilitySystemComponent> ASC;
	bool bDone = false;
	double TimeoutAt = 0.0;
};

static void CleanupDelegate(FDFMeleeMem& M, UAbilitySystemComponent* ASC)
{
	if (M.Handle.IsValid() && ASC)
	{
		ASC->OnAbilityEnded.Remove(M.Handle);
		M.Handle.Reset();
	}
}
} // namespace

UDFBTTask_MeleeAttack::UDFBTTask_MeleeAttack()
{
	NodeName = TEXT("DF MeleeAttack");
	bNotifyTick = true;
	RequiredTag = FDFGameplayTags::Ability_Attack_Melee;
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

uint16 UDFBTTask_MeleeAttack::GetInstanceMemorySize() const
{
	return sizeof(FDFMeleeMem);
}

EBTNodeResult::Type UDFBTTask_MeleeAttack::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* const NodeMemory)
{
	AAIController* const AI = OwnerComp.GetAIOwner();
	ADFEnemyBase* const Enemy = AI ? Cast<ADFEnemyBase>(AI->GetPawn()) : nullptr;
	if (!IsValid(Enemy) || !IsValid(Enemy->GetAbilitySystemComponent()))
	{
		return EBTNodeResult::Failed;
	}
	UAbilitySystemComponent* const ASC = Enemy->GetAbilitySystemComponent();
	if (!RequiredTag.IsValid())
	{
		return EBTNodeResult::Failed;
	}
	FGameplayTagContainer C;
	C.AddTag(RequiredTag);
	if (!ASC->TryActivateAbilitiesByTag(C, true))
	{
		return EBTNodeResult::Failed;
	}

	new (NodeMemory) FDFMeleeMem();
	auto* const M = reinterpret_cast<FDFMeleeMem*>(NodeMemory);
	M->ASC = ASC;
	if (UWorld* const W = OwnerComp.GetWorld())
	{
		M->TimeoutAt = W->GetTimeSeconds() + FMath::Max(0.1, AbilityEndTimeLimit);
	}

	const TWeakObjectPtr<UAbilitySystemComponent> WASC(ASC);
	const TWeakObjectPtr<UBehaviorTreeComponent> WTree(&OwnerComp);
	const FGameplayTag TagRequired = RequiredTag;
	M->Handle = ASC->OnAbilityEnded.AddLambda(
		[this, M, WTree, WASC, TagRequired](
			const FAbilityEndedData& P)
		{
			if (M->bDone || !WTree.IsValid())
			{
				return;
			}
			UAbilitySystemComponent* const Src = WASC.Get();
			if (!Src)
			{
				return;
			}
			UGameplayAbility* const Ended = P.AbilityThatEnded;
			if (!IsValid(Ended))
			{
				return;
			}
			const UGameplayAbility* const CDO = Ended->GetClass()
				? Ended->GetClass()->GetDefaultObject<UGameplayAbility>() : nullptr;
			if (TagRequired.IsValid() && CDO && !CDO->AbilityTags.HasTag(TagRequired))
			{
				return;
			}
			(*M).bDone = true;
			CleanupDelegate(*M, Src);
			if (UBehaviorTreeComponent* const Tree = WTree.Get())
			{
				this->FinishLatentTask(*Tree, EBTNodeResult::Succeeded);
			}
		});

	if (!M->Handle.IsValid())
	{
		return EBTNodeResult::Failed;
	}
	return EBTNodeResult::InProgress;
}

void UDFBTTask_MeleeAttack::TickTask(
	UBehaviorTreeComponent& OwnerComp, uint8* const NodeMemory, const float /*DeltaSeconds*/)
{
	auto* const M = reinterpret_cast<FDFMeleeMem*>(NodeMemory);
	if (M->bDone)
	{
		return;
	}
	UWorld* const W = OwnerComp.GetWorld();
	if (!W || M->TimeoutAt <= 0.0)
	{
		return;
	}
	if (W->GetTimeSeconds() < M->TimeoutAt)
	{
		return;
	}
	M->bDone = true;
	if (UAbilitySystemComponent* const ASC = M->ASC.Get())
	{
		CleanupDelegate(*M, ASC);
	}
	FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
}

EBTNodeResult::Type UDFBTTask_MeleeAttack::AbortTask(
	UBehaviorTreeComponent& OwnerComp, uint8* const NodeMemory)
{
	auto* const M = reinterpret_cast<FDFMeleeMem*>(NodeMemory);
	if (UAbilitySystemComponent* const ASC = M->ASC.Get())
	{
		CleanupDelegate(*M, ASC);
	}
	return EBTNodeResult::Aborted;
}
