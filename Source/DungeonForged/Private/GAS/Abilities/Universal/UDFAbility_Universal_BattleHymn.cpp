#include "GAS/Abilities/Universal/UDFAbility_Universal_BattleHymn.h"
#include "AbilitySystemComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Buff_BattleHymn.h"
#include "GAS/Effects/UGE_Cooldown_Universal_BattleHymn.h"
#include "GameplayEffect.h"

UDFAbility_Universal_BattleHymn::UDFAbility_Universal_BattleHymn()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	AbilityCost_Mana = 40.f;
	BaseCooldown = 90.f;
	BattleHymnBuffClass = UGE_Buff_BattleHymn::StaticClass();
	CooldownGameplayEffectClass = UGE_Cooldown_Universal_BattleHymn::StaticClass();
}

void UDFAbility_Universal_BattleHymn::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Universal_BattleHymn);
		ActivationOwnedTags.AddTag(FDFGameplayTags::State_Casting);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
	}
}

void UDFAbility_Universal_BattleHymn::ActivateAbility(
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
	if (UAbilitySystemComponent* const ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		if (ASC->GetOwner() && ASC->GetOwner()->HasAuthority() && BattleHymnBuffClass)
		{
			const FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
			const FGameplayEffectSpecHandle S = ASC->MakeOutgoingSpec(BattleHymnBuffClass, 1.f, Ctx);
			if (S.IsValid() && S.Data.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*S.Data.Get());
			}
		}
	}
	K2_CommitAbilityCooldown(true, true);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
