// Source/DungeonForged/Private/GAS/Abilities/Warrior/UDFAbility_Warrior_HeavyAttack.cpp
#include "GAS/Abilities/Warrior/UDFAbility_Warrior_HeavyAttack.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Combat/UDFComboComponent.h"
#include "Combat/UDFMeleeTraceComponent.h"
#include "GAS/DFGameplayTags.h"

UDFAbility_Warrior_HeavyAttack::UDFAbility_Warrior_HeavyAttack()
{
	bRetriggerInstancedAbility = false;
	AbilityCost_Mana = 0.f;
	AbilityCost_Stamina = 15.f;
	BaseCooldown = 0.f;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UDFAbility_Warrior_HeavyAttack::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Warrior_HeavyAttack);
		AbilityTags.AddTag(FDFGameplayTags::Ability_Attack);
		AbilityTags.AddTag(FDFGameplayTags::Ability_Attack_Melee);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Stunned);
		ActivationOwnedTags.AddTag(FDFGameplayTags::State_Attacking);
	}
}

void UDFAbility_Warrior_HeavyAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	(void)TriggerEventData;
	if (!ActorInfo)
	{
		EndAbility(Handle, nullptr, ActivationInfo, true, true);
		return;
	}

	ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(ActorInfo->AvatarActor.Get());
	UDFComboComponent* const Combo = PC ? PC->Combo : nullptr;
	if (Combo)
	{
		AbilityCost_Stamina = Combo->HeavyStaminaCost;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo, nullptr))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (ASC->GetOwner() && ASC->GetOwner()->HasAuthority())
		{
			ApplyResourceCostsToOwner(ASC);
		}
	}

	UAnimMontage* MontToPlay = nullptr;
	if (Combo)
	{
		MontToPlay = Combo->ResolveHeavyAttackMontage();
	}
	if (!MontToPlay)
	{
		MontToPlay = AbilityMontage.Get();
	}

	AActor* const Avatar = ActorInfo->AvatarActor.Get();
	if (!MontToPlay || !Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (Combo)
	{
		Combo->NotifyHeavyAbilitySwingMontageStarted(MontToPlay);
	}

	if (UAbilityTask_PlayMontageAndWait* const PlayTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MontToPlay, 1.f, NAME_None, true, 1.f, 0.f, true))
	{
		PlayTask->OnCompleted.AddDynamic(this, &UDFAbility_Warrior_HeavyAttack::OnMontageEnd);
		PlayTask->OnBlendOut.AddDynamic(this, &UDFAbility_Warrior_HeavyAttack::OnMontageEnd);
		PlayTask->OnInterrupted.AddDynamic(this, &UDFAbility_Warrior_HeavyAttack::OnMontageEnd);
		PlayTask->OnCancelled.AddDynamic(this, &UDFAbility_Warrior_HeavyAttack::OnMontageEnd);
		PlayTask->ReadyForActivation();
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UDFAbility_Warrior_HeavyAttack::OnMontageEnd()
{
	if (ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (PC->MeleeTrace)
		{
			PC->MeleeTrace->EndTrace();
		}
		if (UDFComboComponent* const Combo = PC->Combo)
		{
			Combo->NotifyAbilitySwingMontagePlaybackEnded();
		}
	}
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
