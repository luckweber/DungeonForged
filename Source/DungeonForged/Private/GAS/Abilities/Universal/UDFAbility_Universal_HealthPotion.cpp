#include "GAS/Abilities/Universal/UDFAbility_Universal_HealthPotion.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Cooldown_Universal_HealthPotion.h"
#include "GAS/UDFAttributeSet.h"

UDFAbility_Universal_HealthPotion::UDFAbility_Universal_HealthPotion()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	BaseCooldown = 30.f;
	AbilityCost_Mana = 0.f;
	AbilityCost_Stamina = 0.f;
	CooldownGameplayEffectClass = UGE_Cooldown_Universal_HealthPotion::StaticClass();
}

void UDFAbility_Universal_HealthPotion::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Universal_HealthPotion);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
	}
}

void UDFAbility_Universal_HealthPotion::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}
	UAbilitySystemComponent* const ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC->GetOwner() || !ASC->GetOwner()->HasAuthority())
	{
		return;
	}
	if (FGameplayAbilitySpec* const Mutable = ASC->FindAbilitySpecFromHandle(Spec.Handle))
	{
		if (Mutable->Level <= 0)
		{
			Mutable->Level = FMath::Max(1, StartingCharges);
			ASC->MarkAbilitySpecDirty(*Mutable);
		}
	}
}

bool UDFAbility_Universal_HealthPotion::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	if (const UAbilitySystemComponent* const ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		if (const FGameplayAbilitySpec* const Spec = ASC->FindAbilitySpecFromHandle(Handle))
		{
			return Spec->Level > 0;
		}
	}
	return false;
}

void UDFAbility_Universal_HealthPotion::BroadcastCharges(
	UAbilitySystemComponent& ASC, const FGameplayAbilitySpecHandle Handle) const
{
	if (!FDFGameplayTags::Event_Universal_HealthPotion_Charges.IsValid())
	{
		return;
	}
	int32 Remaining = 0;
	if (const FGameplayAbilitySpec* const Spec = ASC.FindAbilitySpecFromHandle(Handle))
	{
		Remaining = Spec->Level;
	}
	FGameplayEventData Payload;
	Payload.EventTag = FDFGameplayTags::Event_Universal_HealthPotion_Charges;
	Payload.EventMagnitude = static_cast<float>(Remaining);
	if (AActor* const Av = ASC.GetAvatarActor())
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Av, Payload.EventTag, Payload);
	}
}

void UDFAbility_Universal_HealthPotion::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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
	UAbilitySystemComponent* const ASC = ActorInfo->AbilitySystemComponent.Get();
	if (ASC && ASC->GetOwner() && ASC->GetOwner()->HasAuthority())
	{
		const float MaxH = FMath::Max(1.f, ASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute()));
		const float Heal = MaxH * HealFractionOfMaxHealth;
		if (Heal > KINDA_SMALL_NUMBER)
		{
			ASC->ApplyModToAttribute(UDFAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, Heal);
		}
		if (FGameplayAbilitySpec* const Spec = ASC->FindAbilitySpecFromHandle(Handle))
		{
			Spec->Level = FMath::Max(0, Spec->Level - 1);
			ASC->MarkAbilitySpecDirty(*Spec);
		}
		BroadcastCharges(*ASC, Handle);
	}
	K2_CommitAbilityCooldown(true, true);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
