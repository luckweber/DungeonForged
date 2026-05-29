// Source/DungeonForged/Private/GAS/DFGameplayCueRegistration.cpp
#include "GAS/DFGameplayCueRegistration.h"

#include "AbilitySystemGlobals.h"
#include "GAS/Cues/UDFGameplayCueNotify_CombatImpact.h"
#include "GAS/Cues/UDFGameplayCueNotify_EnemyDeath.h"
#include "GAS/Cues/UDFGameplayCueNotify_ElementReaction.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFGameplayCueRegistry.h"
#include "GameplayCueManager.h"
#include "GameplayCueSet.h"

namespace DFGameplayCueRegistration
{
static bool GRegisteredNativeCues = false;

static void RegisterNativeCues()
{
	if (GRegisteredNativeCues)
	{
		return;
	}
	if (!FDFGameplayTags::GameplayCue_Enemy_Death.IsValid()
		&& !FDFGameplayTags::GameplayCue_Combat_Impact_Light.IsValid())
	{
		return;
	}

	UAbilitySystemGlobals& GASGlobals = UAbilitySystemGlobals::Get();
	GASGlobals.InitGlobalData();

	UGameplayCueManager* const CueManager = GASGlobals.GetGameplayCueManager();
	if (!CueManager)
	{
		return;
	}
	if (!CueManager->GetRuntimeCueSet())
	{
		CueManager->InitializeRuntimeObjectLibrary();
	}
	UGameplayCueSet* const CueSet = CueManager->GetRuntimeCueSet();
	if (!CueSet)
	{
		return;
	}

	const TSubclassOf<UGameplayCueNotify_Static> DeathClass = UDFGameplayCueNotify_EnemyDeath::StaticClass();
	const TSubclassOf<UGameplayCueNotify_Static> CombatClass = UDFGameplayCueNotify_CombatImpact::StaticClass();
	const TSubclassOf<UGameplayCueNotify_Static> ElementClass = UDFGameplayCueNotify_ElementReaction::StaticClass();
	auto HasNativeCue = [CueSet](const FGameplayTag& Tag, const UClass* const Class) -> bool
	{
		for (const FGameplayCueNotifyData& Existing : CueSet->GameplayCueData)
		{
			if (Existing.GameplayCueTag == Tag && Existing.LoadedGameplayCueClass == Class)
			{
				return true;
			}
		}
		return false;
	};

	TArray<FGameplayCueReferencePair> Refs;
	const FGameplayTag DeathCueTag = FDFGameplayTags::GameplayCue_Enemy_Death;
	if (DeathCueTag.IsValid() && !HasNativeCue(DeathCueTag, DeathClass))
	{
		Refs.Emplace(DeathCueTag, FSoftObjectPath(DeathClass));
	}
	if (FDFGameplayTags::GameplayCue_Combat_Impact_Light.IsValid()
		&& !HasNativeCue(FDFGameplayTags::GameplayCue_Combat_Impact_Light, CombatClass))
	{
		UDFGameplayCueRegistry::AppendNativeCombatCueReferences(Refs);
	}
	if (FDFGameplayTags::GameplayCue_Element_Reaction_Melt.IsValid()
		&& !HasNativeCue(FDFGameplayTags::GameplayCue_Element_Reaction_Melt, ElementClass))
	{
		UDFGameplayCueRegistry::AppendNativeElementCueReferences(Refs);
	}
	if (!Refs.IsEmpty())
	{
		CueSet->AddCues(Refs);
	}
	GRegisteredNativeCues = true;
}

void RegisterNativeGameplayCues()
{
	if (!FDFGameplayTags::GameplayCue_Enemy_Death.IsValid()
		|| !FDFGameplayTags::GameplayCue_Combat_Impact_Light.IsValid())
	{
		FDFGameplayTags::RegisterGameplayTags();
	}
	RegisterNativeCues();
}
} // namespace DFGameplayCueRegistration
