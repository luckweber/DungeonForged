// Source/DungeonForged/Private/GAS/Effects/UDFMMC_NegateDataCost.cpp
#include "GAS/Effects/UDFMMC_NegateDataCost.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffectTypes.h"

float UDFMMC_NegateDataCost::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTag Tag = FDFGameplayTags::Data_Cost.IsValid()
		? FDFGameplayTags::Data_Cost
		: FGameplayTag::RequestGameplayTag(FName("Data.Cost"), false);
	if (!Tag.IsValid())
	{
		return 0.f;
	}
	return -FMath::Max(0.f, GetSetByCallerMagnitudeByTag(Spec, Tag));
}
