// Source/DungeonForged/Private/Combat/UDFCombatEventsLibrary.cpp
#include "Combat/UDFCombatEventsLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/DFGameplayTags.h"
#include "DungeonForgedModule.h"

static FOnDFDamageDealt GOnDFDamageDealt;

FOnDFDamageDealt& UDFCombatEventsLibrary::GetOnDamageDealtDelegate()
{
	return GOnDFDamageDealt;
}

void UDFCombatEventsLibrary::BroadcastDamageDealt(const FDFDamageDealtContext& Context)
{
	GOnDFDamageDealt.Broadcast(Context);
	UE_LOG(LogDFFeel, Verbose, TEXT("[DamageDealt] %.1f Crit=%d Lethal=%d %s -> %s"),
		Context.Magnitude, Context.bIsCrit ? 1 : 0, Context.bWasLethal ? 1 : 0,
		*GetNameSafe(Context.Source), *GetNameSafe(Context.Victim));
}

void UDFCombatEventsLibrary::NotifyFinisherTargetAvailable(
	UObject* const WorldContextObject,
	AActor* const Attacker,
	AActor* const Victim)
{
	(void)WorldContextObject;
	if (!Attacker || !Victim)
	{
		return;
	}
	if (UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Attacker))
	{
		if (FDFGameplayTags::State_Combat_FinisherReady.IsValid())
		{
			ASC->AddLooseGameplayTag(FDFGameplayTags::State_Combat_FinisherReady, 1);
		}
	}
	if (FDFGameplayTags::Event_Combat_Finisher_Available.IsValid())
	{
		FGameplayEventData Payload;
		Payload.EventTag = FDFGameplayTags::Event_Combat_Finisher_Available;
		Payload.Instigator = Attacker;
		Payload.Target = Victim;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Attacker, FDFGameplayTags::Event_Combat_Finisher_Available, Payload);
	}
}
