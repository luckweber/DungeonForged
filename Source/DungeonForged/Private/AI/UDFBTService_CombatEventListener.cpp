// Source/DungeonForged/Private/AI/UDFBTService_CombatEventListener.cpp
#include "AI/UDFBTService_CombatEventListener.h"

#include "AI/DFAIKeys.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/ADFEnemyBase.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GAS/DFGameplayTags.h"
#include "DungeonForgedModule.h"

UDFBTService_CombatEventListener::UDFBTService_CombatEventListener()
{
	NodeName = TEXT("DF CombatEventListener");
	Interval = 0.25f;
	RandomDeviation = 0.05f;
	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;
	bCreateNodeInstance = true;
	INIT_SERVICE_NODE_NOTIFY_FLAGS();
}

void UDFBTService_CombatEventListener::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	OnCeaseRelevant(OwnerComp, nullptr);
	RecoverEndWorldTime = 0.0;

	AAIController* const AI = OwnerComp.GetAIOwner();
	ADFEnemyBase* const Enemy = AI ? Cast<ADFEnemyBase>(AI->GetPawn()) : nullptr;
	UAbilitySystemComponent* const ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
	if (!ASC || !FDFGameplayTags::Event_Combat_Parry_Triggered.IsValid())
	{
		return;
	}
	BoundAI = AI;
	BoundASC = ASC;
	ParryDelegateHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(
		FDFGameplayTags::Event_Combat_Parry_Triggered).AddUObject(
		this, &UDFBTService_CombatEventListener::OnParryTriggered);
}

void UDFBTService_CombatEventListener::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	(void)OwnerComp;
	if (UAbilitySystemComponent* const ASC = BoundASC.Get())
	{
		if (ParryDelegateHandle.IsValid() && FDFGameplayTags::Event_Combat_Parry_Triggered.IsValid())
		{
			ASC->GenericGameplayEventCallbacks.FindOrAdd(FDFGameplayTags::Event_Combat_Parry_Triggered)
				.Remove(ParryDelegateHandle);
		}
	}
	ParryDelegateHandle.Reset();
	BoundASC = nullptr;
	BoundAI = nullptr;
	RecoverEndWorldTime = 0.0;
}

void UDFBTService_CombatEventListener::TickNode(
	UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/, const float /*DeltaSeconds*/)
{
	if (RecoverEndWorldTime <= 0.0)
	{
		return;
	}
	UWorld* const World = OwnerComp.GetWorld();
	UBlackboardComponent* const BB = OwnerComp.GetBlackboardComponent();
	if (!World || !IsValid(BB) || World->GetTimeSeconds() < RecoverEndWorldTime)
	{
		return;
	}
	RecoverEndWorldTime = 0.0;
	BB->SetValueAsBool(DFAIKeys::bWasParried, false);
	BB->SetValueAsEnum(DFAIKeys::CombatState, static_cast<uint8>(EADFAICombatState::Chase));
}

void UDFBTService_CombatEventListener::OnParryTriggered(const FGameplayEventData* const Payload)
{
	if (!Payload || !BoundASC.IsValid() || !BoundAI.IsValid())
	{
		return;
	}
	if (Payload->Target.Get() != BoundASC->GetAvatarActor())
	{
		return;
	}
	UBlackboardComponent* const BB = BoundAI->GetBlackboardComponent();
	if (!IsValid(BB))
	{
		return;
	}
	BB->SetValueAsBool(DFAIKeys::bWasParried, true);
	BB->SetValueAsEnum(DFAIKeys::CombatState, static_cast<uint8>(EADFAICombatState::Recover));
	if (UWorld* const World = BoundAI->GetWorld())
	{
		RecoverEndWorldTime = World->GetTimeSeconds() + FMath::Max(0.1f, ParryRecoveryDuration);
	}
	UE_LOG(LogDFAI, Verbose, TEXT("[AI] %s parried — Recover %.1fs"),
		*GetNameSafe(BoundASC->GetAvatarActor()), ParryRecoveryDuration);
}
