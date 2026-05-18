// Source/DungeonForged/Private/GAS/Abilities/UDFAbility_Death.cpp
#include "GAS/Abilities/UDFAbility_Death.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "Animation/DFDeathAnimation.h"
#include "Characters/ADFEnemyBase.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GAS/DFGameplayTags.h"

UUDFAbility_Death::UUDFAbility_Death()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationPolicy = EAbilityActivationPolicy::Passive;
	bRetriggerInstancedAbility = false;
}

void UUDFAbility_Death::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		if (FDFGameplayTags::Event_Death.IsValid())
		{
			FAbilityTriggerData TriggerData;
			TriggerData.TriggerTag = FDFGameplayTags::Event_Death;
			TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
			AbilityTriggers.Add(TriggerData);
		}
		if (FDFGameplayTags::State_Dead.IsValid())
		{
			ActivationOwnedTags.AddTag(FDFGameplayTags::State_Dead);
		}
	}
}

bool UUDFAbility_Death::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return false;
	}
	if (const ADFEnemyBase* const Enemy = Cast<ADFEnemyBase>(ActorInfo->AvatarActor.Get()))
	{
		return Enemy->HasDied() || Enemy->IsInDeathFlow();
	}
	if (const ADFPlayerCharacter* const Player = Cast<ADFPlayerCharacter>(ActorInfo->AvatarActor.Get()))
	{
		return Player->IsPlayerDeathHandled();
	}
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UUDFAbility_Death::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	(void)TriggerEventData;
	if (!ActorInfo)
	{
		EndAbility(Handle, nullptr, ActivationInfo, true, true);
		return;
	}
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo, nullptr))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PendingDeathTasks = 0;
	bDeathLootEventHandled = false;
	bMontageCompletionHandled = false;

	if (UAbilitySystemComponent* const ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		ASC->CancelAllAbilities(this);
	}

	ApplyDeathState();
	OnDeathFlowStarted();

	if (DeathEventTag.IsValid())
	{
		if (UAbilityTask_WaitGameplayEvent* const EventTask =
				UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, DeathEventTag, nullptr, true, true))
		{
			EventTask->EventReceived.AddDynamic(this, &UUDFAbility_Death::OnDeathGameplayEvent);
			EventTask->ReadyForActivation();
		}
	}

	StartDeathMontagePipeline();
}

UAnimMontage* UUDFAbility_Death::ResolveDeathMontage() const
{
	return AbilityMontage;
}

void UUDFAbility_Death::ApplyDeathState()
{
}

void UUDFAbility_Death::OnDeathFlowStarted()
{
}

void UUDFAbility_Death::OnDeathMontagePipelineFinished(bool bInterrupted)
{
	(void)bInterrupted;
}

void UUDFAbility_Death::StartDeathMontagePipeline()
{
	UAnimMontage* const Montage = ResolveDeathMontage();
	if (!Montage)
	{
		FinishDeathMontagePipeline(false);
		return;
	}

	if (UAbilityTask_PlayMontageAndWait* const MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, Montage, 1.f, NAME_None, false, 1.f, 0.f, true))
	{
		++PendingDeathTasks;
		MontageTask->OnCompleted.AddDynamic(this, &UUDFAbility_Death::OnDeathMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UUDFAbility_Death::OnDeathMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &UUDFAbility_Death::OnDeathMontageCancelled);
		if (bFinishDeathOnMontageBlendOut)
		{
			MontageTask->OnBlendOut.AddDynamic(this, &UUDFAbility_Death::OnDeathMontageBlendOut);
		}
		MontageTask->ReadyForActivation();
		return;
	}

	const float Played = PlayAbilityMontage(1.f, NAME_None);
	float WaitTime = (Played > 0.01f) ? Played : Montage->GetPlayLength();
	if (WaitTime <= 0.01f)
	{
		WaitTime = 0.5f;
	}
	if (UAbilityTask_WaitDelay* const Delay = UAbilityTask_WaitDelay::WaitDelay(this, WaitTime))
	{
		++PendingDeathTasks;
		Delay->OnFinish.AddDynamic(this, &UUDFAbility_Death::OnDeathMontageDelayFinished);
		Delay->ReadyForActivation();
	}
	else
	{
		FinishDeathMontagePipeline(true);
	}
}

void UUDFAbility_Death::FinishDeathMontagePipeline(const bool bInterrupted)
{
	if (bMontageCompletionHandled)
	{
		return;
	}
	bMontageCompletionHandled = true;

	OnDeathMontagePipelineFinished(bInterrupted);
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UUDFAbility_Death::OnDeathMontageCompleted()
{
	TryFinishDeathAbility(false);
}

void UUDFAbility_Death::OnDeathMontageInterrupted()
{
	TryFinishDeathAbility(true);
}

void UUDFAbility_Death::OnDeathMontageCancelled()
{
	TryFinishDeathAbility(true);
}

void UUDFAbility_Death::OnDeathMontageBlendOut()
{
	TryFinishDeathAbility(false);
}

void UUDFAbility_Death::OnDeathMontageDelayFinished()
{
	TryFinishDeathAbility(false);
}

void UUDFAbility_Death::OnDeathGameplayEvent(FGameplayEventData Payload)
{
	(void)Payload;
	bDeathLootEventHandled = true;
}

void UUDFAbility_Death::TryFinishDeathAbility(const bool bInterrupted)
{
	if (PendingDeathTasks > 0)
	{
		--PendingDeathTasks;
	}
	if (PendingDeathTasks > 0)
	{
		return;
	}
	FinishDeathMontagePipeline(bInterrupted);
}
