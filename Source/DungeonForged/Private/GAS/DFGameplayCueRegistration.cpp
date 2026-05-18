// Source/DungeonForged/Private/GAS/DFGameplayCueRegistration.cpp
#include "GAS/DFGameplayCueRegistration.h"

#include "AbilitySystemGlobals.h"
#include "GAS/Cues/UDFGameplayCueNotify_EnemyDeath.h"
#include "GAS/DFGameplayTags.h"
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
	if (!FDFGameplayTags::GameplayCue_Enemy_Death.IsValid())
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

	const FGameplayTag DeathCueTag = FDFGameplayTags::GameplayCue_Enemy_Death;
	for (const FGameplayCueNotifyData& Existing : CueSet->GameplayCueData)
	{
		if (Existing.GameplayCueTag == DeathCueTag
			&& Existing.LoadedGameplayCueClass == UDFGameplayCueNotify_EnemyDeath::StaticClass())
		{
			GRegisteredNativeCues = true;
			return;
		}
	}

	TArray<FGameplayCueReferencePair> Refs;
	Refs.Emplace(DeathCueTag, FSoftObjectPath(UDFGameplayCueNotify_EnemyDeath::StaticClass()));
	CueSet->AddCues(Refs);
	GRegisteredNativeCues = true;
}

void RegisterNativeGameplayCues()
{
	if (!FDFGameplayTags::GameplayCue_Enemy_Death.IsValid())
	{
		FDFGameplayTags::RegisterGameplayTags();
	}
	RegisterNativeCues();
}
} // namespace DFGameplayCueRegistration
