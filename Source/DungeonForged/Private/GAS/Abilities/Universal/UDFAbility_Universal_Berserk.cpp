#include "GAS/Abilities/Universal/UDFAbility_Universal_Berserk.h"
#include "AbilitySystemComponent.h"
#include "Characters/ADFPlayerCharacter.h"
#include "FX/UDFScreenEffectsComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Buff_Berserk.h"
#include "GAS/Effects/UGE_Cooldown_Universal_Berserk.h"
#include "GameplayEffect.h"

UDFAbility_Universal_Berserk::UDFAbility_Universal_Berserk()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	AbilityCost_Stamina = 50.f;
	BaseCooldown = 120.f;
	BerserkBuffClass = UGE_Buff_Berserk::StaticClass();
	CooldownGameplayEffectClass = UGE_Cooldown_Universal_Berserk::StaticClass();
}

void UDFAbility_Universal_Berserk::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Universal_Berserk);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
	}
}

void UDFAbility_Universal_Berserk::SetBerserkPresentation(const bool bActive) const
{
	if (const ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UDFScreenEffectsComponent* const FX = PC->FindComponentByClass<UDFScreenEffectsComponent>())
		{
			FX->BerserkSetActive(bActive);
		}
	}
}

void UDFAbility_Universal_Berserk::ActivateAbility(
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
	SetBerserkPresentation(true);
	if (UAbilitySystemComponent* const ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		if (ASC->GetOwner() && ASC->GetOwner()->HasAuthority() && BerserkBuffClass)
		{
			const FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
			const FGameplayEffectSpecHandle S = ASC->MakeOutgoingSpec(BerserkBuffClass, 1.f, Ctx);
			if (S.IsValid() && S.Data.IsValid())
			{
				ActiveBuffHandle = ASC->ApplyGameplayEffectSpecToSelf(*S.Data.Get());
			}
		}
	}
	K2_CommitAbilityCooldown(true, true);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
