// Source/DungeonForged/Private/GAS/UDFPassivesGASEvents.cpp
#include "GAS/UDFPassivesGASEvents.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"

static AActor* ResolveEventActorForASC(UAbilitySystemComponent* const ASC)
{
	if (!ASC)
	{
		return nullptr;
	}
	if (AActor* O = Cast<AActor>(ASC->GetOwner()))
	{
		if (O->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
		{
			return O;
		}
	}
	if (APawn* P = Cast<APawn>(ASC->GetOwnerActor()))
	{
		if (APlayerState* const PS = P->GetPlayerState())
		{
			if (PS->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
			{
				return PS;
			}
		}
		return P;
	}
	return nullptr;
}

namespace UDFPassivesGASEvents
{
void DispatchHitReceived(UAbilitySystemComponent* const VictimASC, const FGameplayEffectSpec& Spec, const float Damage)
{
	AActor* const Vict = ResolveEventActorForASC(VictimASC);
	if (!Vict || !FDFGameplayTags::Event_Hit_Received.IsValid())
	{
		return;
	}
	FGameplayEventData P;
	P.EventMagnitude = Damage;
	P.ContextHandle = Spec.GetContext();
	P.Instigator = const_cast<AActor*>(Spec.GetContext().GetEffectCauser());
	if (!P.Instigator)
	{
		P.Instigator = const_cast<AActor*>(Spec.GetContext().GetInstigator());
	}
	P.Target = Vict;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Vict, FDFGameplayTags::Event_Hit_Received, P);
}

void DispatchLethalAbilityKillIfAny(UAbilitySystemComponent* const VictimASC, const FGameplayEffectSpec& Spec)
{
	if (!VictimASC || !FDFGameplayTags::Event_Ability_Kill.IsValid() || !UDFAttributeSet::GetHealthAttribute().IsValid())
	{
		return;
	}
	const float Hp = VictimASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute());
	if (Hp > KINDA_SMALL_NUMBER)
	{
		return;
	}
	const FGameplayEffectContextHandle& Cx = Spec.GetContext();
	if (!Cx.IsValid())
	{
		return;
	}
	if (!Cx.GetAbility())
	{
		return;
	}
	const AActor* const Vict = Cast<AActor>(VictimASC->GetOwner());
	AActor* Killer = Cx.GetEffectCauser() ? Cx.GetEffectCauser() : Cx.GetInstigator();
	if (!Killer)
	{
		return;
	}
	AActor* KillerRoute = ResolveEventActorForASC(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Killer));
	if (!KillerRoute)
	{
		KillerRoute = Killer;
	}
	FGameplayEventData P;
	P.ContextHandle = Cx;
	P.Instigator = Killer;
	P.Target = const_cast<AActor*>(Vict);
	P.EventMagnitude = 0.f;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(KillerRoute, FDFGameplayTags::Event_Ability_Kill, P);
}

void DispatchRogueBleedApplied(
	AActor* const InstigatorAvatar, AActor* const Victim, UAbilitySystemComponent* const InstigatorASC)
{
	if (!InstigatorASC || !InstigatorAvatar || !FDFGameplayTags::Event_Passive_Rogue_BleedApplied.IsValid())
	{
		return;
	}
	AActor* const Route = ResolveEventActorForASC(InstigatorASC);
	if (!Route)
	{
		return;
	}
	FGameplayEventData P;
	P.Instigator = InstigatorAvatar;
	P.Target = Victim;
	P.EventMagnitude = 0.f;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Route, FDFGameplayTags::Event_Passive_Rogue_BleedApplied, P);
}

} // namespace UDFPassivesGASEvents
