// Source/DungeonForged/Private/GAS/Abilities/Warrior/UDFAbility_Warrior_MeleeSwing.cpp
#include "GAS/Abilities/Warrior/UDFAbility_Warrior_MeleeSwing.h"

#include "Abilities/GameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Combat/UDFComboComponent.h"
#include "Combat/UDFMeleeAimComponent.h"
#include "Combat/UDFMeleeTraceComponent.h"
#include "GameFramework/Actor.h"
#include "GAS/DFGameplayTags.h"

UDFAbility_Warrior_MeleeSwing::UDFAbility_Warrior_MeleeSwing()
{
	bRetriggerInstancedAbility = false;
	AbilityCost_Mana = 0.f;
	AbilityCost_Stamina = 6.f;
	BaseCooldown = 0.f;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UDFAbility_Warrior_MeleeSwing::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Warrior_MeleeSwing);
		AbilityTags.AddTag(FDFGameplayTags::Ability_Attack);
		AbilityTags.AddTag(FDFGameplayTags::Ability_Attack_Melee);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Stunned);
		ActivationOwnedTags.AddTag(FDFGameplayTags::State_Attacking);
		if (BaseCooldown > 0.f)
		{
			bRetriggerInstancedAbility = false;
			BuildCooldownTagContainer(CachedCooldownTags);
			ActivationBlockedTags.AppendTags(CachedCooldownTags);
		}
	}
}

void UDFAbility_Warrior_MeleeSwing::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
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

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (ASC->GetOwner() && ASC->GetOwner()->HasAuthority())
		{
			ApplyResourceCostsToOwner(ASC);
		}
	}

	AActor* Avatar = ActorInfo->AvatarActor.Get();
	UDFComboComponent* Combo = nullptr;
	UAnimMontage* MontToPlay = nullptr;
	if (ADFPlayerCharacter* PC = Cast<ADFPlayerCharacter>(Avatar))
	{
		Combo = PC->Combo;
		// AAA aim: snap-rotate to face the resolved target (lock-on > soft cone) and commit it so
		// UANS_DFMeleeWarp notify states warp to the same actor for the duration of the swing.
		if (UDFMeleeAimComponent* const Aim = PC->MeleeAim)
		{
			Aim->AcquireAndCommitTarget();
		}
	}
	if (Combo)
	{
		// Directional resolver picks Backward/Side override if the owner's local velocity matches,
		// otherwise falls back to ComboMontages[step].
		MontToPlay = Combo->ResolveDirectionalComboMontage(Combo->CurrentComboStep);
	}
	if (!MontToPlay)
	{
		MontToPlay = AbilityMontage.Get();
	}

	if (!MontToPlay || !Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (Combo)
	{
		Combo->NotifyAbilitySwingMontageStarted(MontToPlay);
	}

	if (UAbilityTask_PlayMontageAndWait* PlayTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MontToPlay, 1.f, NAME_None, true, 1.f, 0.f, true))
	{
		PlayTask->OnCompleted.AddDynamic(this, &UDFAbility_Warrior_MeleeSwing::OnMontageEnd);
		PlayTask->OnBlendOut.AddDynamic(this, &UDFAbility_Warrior_MeleeSwing::OnMontageEnd);
		PlayTask->OnInterrupted.AddDynamic(this, &UDFAbility_Warrior_MeleeSwing::OnMontageEnd);
		PlayTask->OnCancelled.AddDynamic(this, &UDFAbility_Warrior_MeleeSwing::OnMontageEnd);
		PlayTask->ReadyForActivation();
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UDFAbility_Warrior_MeleeSwing::OnMontageEnd()
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
		if (UDFMeleeAimComponent* const Aim = PC->MeleeAim)
		{
			Aim->ReleaseAttackTarget();
		}
	}
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
