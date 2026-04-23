// Source/DungeonForged/Private/GAS/Abilities/DFAbility_Dodge.cpp
#include "GAS/Abilities/DFAbility_Dodge.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Characters/UDFCharacterMovementComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GameFramework/Character.h"

UDFAbility_Dodge::UDFAbility_Dodge()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	AbilityCost_Mana = 0.f;
	AbilityCost_Stamina = 0.f;
	AbilityMontage = nullptr;
}

void UDFAbility_Dodge::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Movement_Dodge);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Stunned);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dodging);
		CancelAbilitiesWithTag.AddTag(FDFGameplayTags::Ability_Attack_Melee);
	}
}

void UDFAbility_Dodge::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
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
	ACharacter* const Char =
		ActorInfo->AvatarActor.IsValid() ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	UDFCharacterMovementComponent* const CMC = Char ? Cast<UDFCharacterMovementComponent>(Char->GetCharacterMovement()) : nullptr;
	if (!CMC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	const FVector DodgeDir = CMC->GetDodgeDirection();
	CMC->PerformDodge(DodgeDir);

	const float D = FMath::Max(0.01f, CMC->DodgeDuration);

	if (DodgeMontage)
	{
		if (UAbilityTask_PlayMontageAndWait* const MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, DodgeMontage, 1.f, NAME_None, true, 1.f, 0.f, true))
		{
			MontageTask->OnCompleted.AddDynamic(this, &UDFAbility_Dodge::OnDodgeMontageCompleted);
			MontageTask->OnInterrupted.AddDynamic(this, &UDFAbility_Dodge::OnDodgeMontageCancelled);
			MontageTask->OnCancelled.AddDynamic(this, &UDFAbility_Dodge::OnDodgeMontageCancelled);
			MontageTask->ReadyForActivation();
		}
	}
	if (UAbilityTask_WaitDelay* const Wait = UAbilityTask_WaitDelay::WaitDelay(this, D))
	{
		Wait->OnFinish.AddDynamic(this, &UDFAbility_Dodge::OnDodgeDurationElapsed);
		Wait->ReadyForActivation();
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UDFAbility_Dodge::OnDodgeDurationElapsed()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UDFAbility_Dodge::OnDodgeMontageCompleted()
{
	// Time authority is WaitDelay; no-op here to avoid double End.
}

void UDFAbility_Dodge::OnDodgeMontageCancelled()
{
	// Same as above.
}
