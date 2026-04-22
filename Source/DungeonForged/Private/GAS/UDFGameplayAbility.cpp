// Source/DungeonForged/Private/GAS/UDFGameplayAbility.cpp

#include "GAS/UDFGameplayAbility.h"
#include "GAS/UDFAttributeSet.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimMontage.h"

UDFGameplayAbility::UDFGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UDFGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!ActorInfo)
	{
		return false;
	}
	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		const FGameplayAttribute Mana = UDFAttributeSet::GetManaAttribute();
		const FGameplayAttribute Stamina = UDFAttributeSet::GetStaminaAttribute();
		if (AbilityCost_Mana > 0.f)
		{
			if (ASC->GetNumericAttribute(Mana) < AbilityCost_Mana)
			{
				return false;
			}
		}
		if (AbilityCost_Stamina > 0.f)
		{
			if (ASC->GetNumericAttribute(Stamina) < AbilityCost_Stamina)
			{
				return false;
			}
		}
	}
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UDFGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
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
	PlayAbilityMontage();
	K2_OnAbilityActivated(Handle, *ActorInfo, ActivationInfo);
}

UAbilitySystemComponent* UDFGameplayAbility::GetASCForActorInfo(const FGameplayAbilityActorInfo& ActorInfo)
{
	return ActorInfo.AbilitySystemComponent.Get();
}

float UDFGameplayAbility::PlayAbilityMontage(float InPlayRate, FName StartSectionName)
{
	if (!AbilityMontage)
	{
		return 0.f;
	}
	(void)UAbilitySystemGlobals::Get();
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		return ASC->PlayMontage(this, GetCurrentActivationInfo(), AbilityMontage, InPlayRate, StartSectionName);
	}
	return 0.f;
}

void UDFGameplayAbility::K2_OnAbilityActivated_Implementation(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo& ActorInfo,
	const FGameplayAbilityActivationInfo& ActivationInfo)
{
}

void UDFGameplayAbility::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject) && !AdditionalAutoMergeTags.IsEmpty())
	{
		AbilityTags.AppendTags(AdditionalAutoMergeTags);
	}
}

void UDFGameplayAbility::ApplyResourceCostsToOwner(UAbilitySystemComponent* ASC) const
{
	if (!ASC)
	{
		return;
	}
	UDFAttributeSet* const Attrs = const_cast<UDFAttributeSet*>(ASC->GetSet<UDFAttributeSet>());
	if (!Attrs)
	{
		return;
	}
	if (AbilityCost_Mana > 0.f)
	{
		Attrs->SetMana(FMath::Max(0.f, Attrs->GetMana() - AbilityCost_Mana));
	}
	if (AbilityCost_Stamina > 0.f)
	{
		Attrs->SetStamina(FMath::Max(0.f, Attrs->GetStamina() - AbilityCost_Stamina));
	}
}
