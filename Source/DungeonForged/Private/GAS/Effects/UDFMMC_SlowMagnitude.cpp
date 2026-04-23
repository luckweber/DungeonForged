// Source/DungeonForged/Private/GAS/Effects/UDFMMC_SlowMagnitude.cpp
#include "GAS/Effects/UDFMMC_SlowMagnitude.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffectTypes.h"

float UDFMMC_SlowMagnitude::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FTagContainerAggregator& T = Spec.CapturedTargetTags;
	const FGameplayTagContainer* const A = T.GetAggregatedTags();
	if (!A)
	{
		return -0.4f;
	}
	const bool bHasSpeedBuff = FDFGameplayTags::Effect_Buff_Speed.IsValid() && A->HasTag(FDFGameplayTags::Effect_Buff_Speed);
	const float Mag = bHasSpeedBuff ? 0.2f : 0.4f;
	return -Mag;
}
