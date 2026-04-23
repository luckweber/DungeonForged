// Source/DungeonForged/Private/GAS/Abilities/DFAbility_Sprint.cpp
#include "GAS/Abilities/DFAbility_Sprint.h"

#include "Characters/UDFCharacterMovementComponent.h"
#include "GAS/DFGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"

UDFAbility_Sprint::UDFAbility_Sprint()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ActivationPolicy = EAbilityActivationPolicy::OnInputStarted;
	AbilityCost_Mana = 0.f;
	AbilityCost_Stamina = 0.f;
}

void UDFAbility_Sprint::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Movement_Sprint);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Stunned);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Rooted);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dodging);
	}
}

void UDFAbility_Sprint::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
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
	CMC->SetSprinting(true, false);
	UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo();
	if (SprintStaminaDrainEffectClass && ASC)
	{
		const FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(SprintStaminaDrainEffectClass, 1.f, Ctx);
		if (Spec.IsValid())
		{
			SprintStaminaDrainHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			CMC->SetSprintStaminaFromGameplayEffect(true);
		}
	}
}

void UDFAbility_Sprint::EndAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility, const bool bWasCancelled)
{
	if (ActorInfo)
	{
		StopSprint(Handle, *ActorInfo, ActivationInfo);
	}
	else
	{
		RemoveDrainEffectFromASC(GetAbilitySystemComponentFromActorInfo());
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UDFAbility_Sprint::InputReleased(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (ActorInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UDFAbility_Sprint::StopSprint(
	const FGameplayAbilitySpecHandle& /*Handle*/, const FGameplayAbilityActorInfo& ActorInfo, const FGameplayAbilityActivationInfo& /*ActivationInfo*/)
{
	ACharacter* const Char =
		ActorInfo.AvatarActor.IsValid() ? Cast<ACharacter>(ActorInfo.AvatarActor.Get()) : nullptr;
	UDFCharacterMovementComponent* const CMC = Char ? Cast<UDFCharacterMovementComponent>(Char->GetCharacterMovement()) : nullptr;
	if (CMC)
	{
		CMC->SetSprinting(false, false);
		CMC->SetSprintStaminaFromGameplayEffect(false);
	}
	RemoveDrainEffectFromASC(ActorInfo.AbilitySystemComponent.Get());
}

void UDFAbility_Sprint::RemoveDrainEffectFromASC(UAbilitySystemComponent* const ASC)
{
	if (!ASC || !SprintStaminaDrainHandle.IsValid())
	{
		SprintStaminaDrainHandle = FActiveGameplayEffectHandle();
		return;
	}
	ASC->RemoveActiveGameplayEffect(SprintStaminaDrainHandle, 0);
	SprintStaminaDrainHandle = FActiveGameplayEffectHandle();
}
