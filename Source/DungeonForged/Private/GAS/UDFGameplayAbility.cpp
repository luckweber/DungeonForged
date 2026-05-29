// Source/DungeonForged/Private/GAS/UDFGameplayAbility.cpp

#include "GAS/UDFGameplayAbility.h"
#include "AI/UDFAINoiseLibrary.h"
#include "Characters/ADFPlayerCharacter.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Cooldown_Base.h"
#include "GAS/Effects/UGE_Cost_Mana_Base.h"
#include "GAS/Effects/UGE_Cost_Stamina_Base.h"
#include "GAS/UDFAttributeSet.h"
#include "Boss/ADFBossBase.h"
#include "Combat/UDFComboComponent.h"
#include "Combat/UDFAbilityGlobalCooldownSubsystem.h"
#include "Data/UDFCombatTuningData.h"
#include "DFAssetManager.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimMontage.h"
#include "GameplayEffect.h"

UDFGameplayAbility::UDFGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
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
		if (bUseGlobalAbilityCooldown)
		{
			float GCD = GlobalAbilityGCDOverride;
			if (GCD <= KINDA_SMALL_NUMBER)
			{
				if (const UDFCombatTuningData* const Tuning = UDFAssetManager::Get().GetCombatTuningData())
				{
					GCD = Tuning->GlobalAbilityGCD;
				}
			}
			if (UWorld* const World = ActorInfo->AvatarActor.IsValid() ? ActorInfo->AvatarActor->GetWorld() : nullptr)
			{
				if (UDFAbilityGlobalCooldownSubsystem* const GCDSys = World->GetSubsystem<UDFAbilityGlobalCooldownSubsystem>())
				{
					if (!GCDSys->IsGlobalCooldownReady(ASC, GCD))
					{
						return false;
					}
				}
			}
		}
	}
	const bool bSuperOk = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
	if (bSuperOk)
	{
		return true;
	}
	if (OptionalRelevantTags && ActorInfo && ActorInfo->AvatarActor.IsValid())
	{
		const FGameplayTag& BlockedTag = UAbilitySystemGlobals::Get().ActivateFailTagsBlockedTag;
		if (BlockedTag.IsValid() && OptionalRelevantTags->HasTag(BlockedTag))
		{
			if (UDFComboComponent* const Combo = ActorInfo->AvatarActor->FindComponentByClass<UDFComboComponent>())
			{
				if (Combo->IsAbilityCancellable(AbilityTags))
				{
					return true;
				}
			}
		}
	}
	return false;
}

void UDFGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (ActorInfo && ActorInfo->AvatarActor.IsValid())
	{
		UDFAINoiseLibrary::ReportAbilityNoise(ActorInfo->AvatarActor.Get());
	}

	if (bUseGlobalAbilityCooldown && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		if (UWorld* const World = ActorInfo->AvatarActor.IsValid() ? ActorInfo->AvatarActor->GetWorld() : nullptr)
		{
			if (UDFAbilityGlobalCooldownSubsystem* const GCDSys = World->GetSubsystem<UDFAbilityGlobalCooldownSubsystem>())
			{
				GCDSys->MarkGlobalCooldownUsed(ActorInfo->AbilitySystemComponent.Get());
			}
		}
	}

	if (!ActorInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo, nullptr))
	{
		if (ActorInfo && ActorInfo->IsNetAuthority() && ActorInfo->AvatarActor.IsValid())
		{
			if (ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(ActorInfo->AvatarActor.Get()))
			{
				PC->Client_NotifyAbilityActivationRejected(GetClass());
			}
		}
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
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
		if (!CostGameplayEffectClass)
		{
			if (AbilityCost_Mana > KINDA_SMALL_NUMBER)
			{
				CostGameplayEffectClass = UGE_Cost_Mana_Base::StaticClass();
			}
			else if (AbilityCost_Stamina > KINDA_SMALL_NUMBER)
			{
				CostGameplayEffectClass = UGE_Cost_Stamina_Base::StaticClass();
			}
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

	float EffectiveCooldown = BaseCooldown;
	if (UAbilitySystemComponent* const ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		if (const UDFAttributeSet* const Attrs = ASC->GetSet<UDFAttributeSet>())
		{
			const float CDRRaw = FMath::Max(0.f, Attrs->GetCooldownReduction());
			const float HardCap = 0.4f;
			const float Excess = FMath::Max(0.f, CDRRaw - HardCap);
			const float ExtraReduction = Excess / (Excess + 0.6f) * 0.1f;
			const float TotalCDR = FMath::Min(0.5f, HardCap + ExtraReduction);
			EffectiveCooldown = BaseCooldown * (1.f - TotalCDR);
		}
	}
	SpecHandle.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Cooldown, EffectiveCooldown);

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

float UDFGameplayAbility::GetAbilityManaCost() const
{
	return AbilityCost_Mana;
}

float UDFGameplayAbility::GetAbilityStaminaCost() const
{
	return AbilityCost_Stamina;
}

bool UDFGameplayAbility::CheckSingleResourceCost(
	const FGameplayAbilityActorInfo* ActorInfo,
	const UAbilitySystemComponent& ASC,
	const TSubclassOf<UGameplayEffect> CostEffectClass,
	const FGameplayAttribute ResourceAttribute,
	const float CostMagnitude,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!CostEffectClass || CostMagnitude <= KINDA_SMALL_NUMBER || !FDFGameplayTags::Data_Cost.IsValid()
		|| !ResourceAttribute.IsValid())
	{
		return true;
	}
	if (!ActorInfo)
	{
		return false;
	}
	if (ASC.GetNumericAttribute(ResourceAttribute) < CostMagnitude)
	{
		if (OptionalRelevantTags)
		{
			const FGameplayTag& CostFail = UAbilitySystemGlobals::Get().ActivateFailCostTag;
			if (CostFail.IsValid())
			{
				OptionalRelevantTags->AddTag(CostFail);
			}
		}
		return false;
	}
	return true;
}

bool UDFGameplayAbility::CheckCost(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return false;
	}
	UAbilitySystemComponent& ASC = *ActorInfo->AbilitySystemComponent.Get();
	const float ManaCost = GetAbilityManaCost();
	const float StaminaCost = GetAbilityStaminaCost();
	if (ManaCost <= KINDA_SMALL_NUMBER && StaminaCost <= KINDA_SMALL_NUMBER)
	{
		return true;
	}
	if (ManaCost > KINDA_SMALL_NUMBER
		&& !CheckSingleResourceCost(
			ActorInfo, ASC, UGE_Cost_Mana_Base::StaticClass(), UDFAttributeSet::GetManaAttribute(), ManaCost,
			OptionalRelevantTags))
	{
		return false;
	}
	if (StaminaCost > KINDA_SMALL_NUMBER
		&& !CheckSingleResourceCost(
			ActorInfo, ASC, UGE_Cost_Stamina_Base::StaticClass(), UDFAttributeSet::GetStaminaAttribute(),
			StaminaCost, OptionalRelevantTags))
	{
		return false;
	}
	return true;
}

void UDFGameplayAbility::ApplySingleResourceCost(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const TSubclassOf<UGameplayEffect> CostEffectClass,
	const float CostMagnitude) const
{
	if (!CostEffectClass || CostMagnitude <= KINDA_SMALL_NUMBER || !FDFGameplayTags::Data_Cost.IsValid())
	{
		return;
	}
	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
		CostEffectClass, GetAbilityLevel(Handle, ActorInfo));
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return;
	}
	SpecHandle.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Cost, CostMagnitude);
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
}

void UDFGameplayAbility::ApplyCost(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const float ManaCost = GetAbilityManaCost();
	const float StaminaCost = GetAbilityStaminaCost();
	if (ManaCost > KINDA_SMALL_NUMBER)
	{
		ApplySingleResourceCost(Handle, ActorInfo, ActivationInfo, UGE_Cost_Mana_Base::StaticClass(), ManaCost);
	}
	if (StaminaCost > KINDA_SMALL_NUMBER)
	{
		ApplySingleResourceCost(Handle, ActorInfo, ActivationInfo, UGE_Cost_Stamina_Base::StaticClass(), StaminaCost);
	}
}
