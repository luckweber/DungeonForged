// Source/DungeonForged/Private/GAS/Effects/UGE_EnemyDeath.cpp
#include "GAS/Effects/UGE_EnemyDeath.h"

#include "GAS/DFGameplayTags.h"

UGE_EnemyDeath::UGE_EnemyDeath() = default;

void UGE_EnemyDeath::ConfigureEffectCDO()
{
	Super::ConfigureEffectCDO();

	if (FDFGameplayTags::GameplayCue_Enemy_Death.IsValid())
	{
		FGameplayEffectCue Cue;
		Cue.GameplayCueTags.AddTag(FDFGameplayTags::GameplayCue_Enemy_Death);
		GameplayCues.Add(Cue);
	}
}
