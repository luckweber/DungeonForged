// Source/DungeonForged/Private/GAS/Cues/UDFGameplayCueNotify_EnemyDeath.cpp
#include "GAS/Cues/UDFGameplayCueNotify_EnemyDeath.h"

#include "Animation/DFDeathAnimation.h"
#include "Characters/ADFEnemyBase.h"
#include "GAS/DFGameplayTags.h"

UDFGameplayCueNotify_EnemyDeath::UDFGameplayCueNotify_EnemyDeath()
{
	if (FDFGameplayTags::GameplayCue_Enemy_Death.IsValid())
	{
		GameplayCueTag = FDFGameplayTags::GameplayCue_Enemy_Death;
	}
}

bool UDFGameplayCueNotify_EnemyDeath::OnExecute_Implementation(
	AActor* const Target, const FGameplayCueParameters& /*Parameters*/) const
{
	if (!Target || Target->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}
	ADFEnemyBase* const Enemy = Cast<ADFEnemyBase>(Target);
	if (!Enemy)
	{
		return false;
	}
	// Montage is driven by UUDFAbility_Enemy_Death; cue is for optional VFX/SFX only.
	DFDeathAnimation::LogEnemyDeath(2, Enemy, TEXT("GameplayCue.Enemy.Death OnExecute (cosmetic VFX hook)"));
	return true;
}
