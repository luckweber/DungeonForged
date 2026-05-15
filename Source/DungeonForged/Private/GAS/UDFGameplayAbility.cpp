// Source/DungeonForged/Private/GAS/UDFGameplayAbility.cpp

#include "GAS/UDFGameplayAbility.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Cooldown_Base.h"
#include "GAS/UDFAttributeSet.h"
#include "Boss/ADFBossBase.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimMontage.h"
#include "GameplayEffect.h"

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
	if (bSourceObjectMustBeBoss)
	{
		if (!Cast<ADFBossBase>(ActorInfo->AvatarActor.Get()))
		{
			return false;
		}
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
		if (BaseCooldown > 0.f && IsOwnerOnAbilityCooldown(*ASC))
		{
			if (OptionalRelevantTags)
			{
				const FGameplayTag& CooldownTag = UAbilitySystemGlobals::Get().ActivateFailCooldownTag;
				if (CooldownTag.IsValid())
				{
					OptionalRelevantTags->AddTag(CooldownTag);
				}
			}
			return false;
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
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		if (!AdditionalAutoMergeTags.IsEmpty())
		{
			AbilityTags.AppendTags(AdditionalAutoMergeTags);
		}
		if (BaseCooldown > 0.f)
		{
			BuildCooldownTagContainer(CachedCooldownTags);
			ActivationBlockedTags.AppendTags(CachedCooldownTags);
		}
	}
}

void UDFGameplayAbility::EnsureCooldownTagsCached() const
{
	if (CachedCooldownTags.Num() == 0 && BaseCooldown > 0.f)
	{
		BuildCooldownTagContainer(CachedCooldownTags);
	}
}

bool UDFGameplayAbility::IsOwnerOnAbilityCooldown(const UAbilitySystemComponent& ASC) const
{
	if (FDFGameplayTags::Ability_Cooldown.IsValid() && ASC.HasMatchingGameplayTag(FDFGameplayTags::Ability_Cooldown))
	{
		return true;
	}
	EnsureCooldownTagsCached();
	return CachedCooldownTags.Num() > 0 && ASC.HasAnyMatchingGameplayTags(CachedCooldownTags);
}

const FGameplayTagContainer* UDFGameplayAbility::GetCooldownTags() const
{
	EnsureCooldownTagsCached();
	if (CachedCooldownTags.Num() > 0)
	{
		return &CachedCooldownTags;
	}
	return Super::GetCooldownTags();
}

void UDFGameplayAbility::BuildCooldownTagContainer(FGameplayTagContainer& OutTags) const
{
	OutTags.Reset();
	if (!FDFGameplayTags::Ability_Cooldown.IsValid())
	{
		return;
	}
	OutTags.AddTag(FDFGameplayTags::Ability_Cooldown);

	if (const UGameplayEffect* const CooldownGE = GetCooldownGameplayEffect())
	{
		OutTags.AppendTags(CooldownGE->GetGrantedTags());
		if (const UGE_Cooldown_Base* const CooldownBase = Cast<UGE_Cooldown_Base>(CooldownGE))
		{
			if (CooldownBase->CooldownAssociatedAbilityTag.IsValid())
			{
				OutTags.AddTag(CooldownBase->CooldownAssociatedAbilityTag);
			}
		}
	}

	if (OutTags.Num() <= 1)
	{
		OutTags.AppendTags(AbilityTags);
	}
}

void UDFGameplayAbility::ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* const CooldownGE = GetCooldownGameplayEffect();
	if (!CooldownGE || BaseCooldown <= 0.f || !FDFGameplayTags::Data_Cooldown.IsValid())
	{
		Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle =
		MakeOutgoingGameplayEffectSpec(CooldownGE->GetClass(), GetAbilityLevel(Handle, ActorInfo));
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Cooldown, BaseCooldown);

	FGameplayTagContainer CooldownGrantTags;
	BuildCooldownTagContainer(CooldownGrantTags);
	SpecHandle.Data->DynamicGrantedTags.AppendTags(CooldownGrantTags);

	if (UAbilitySystemComponent* const ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		if (IsOwnerOnAbilityCooldown(*ASC))
		{
			return;
		}
	}

	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
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
