// Source/DungeonForged/Private/GAS/Effects/UDFMMC_NegateDataMagnitude.cpp
#include "GAS/Effects/UDFMMC_NegateDataMagnitude.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffectTypes.h"

float UDFMMC_NegateDataMagnitude::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTag Tag = FDFGameplayTags::Data_Magnitude.IsValid()
		? FDFGameplayTags::Data_Magnitude
		: FGameplayTag::RequestGameplayTag(FName("Data.Magnitude"), false);
	if (!Tag.IsValid())
	{
		return 0.f;
	}
	return -FMath::Max(0.f, GetSetByCallerMagnitudeByTag(Spec, Tag));
}
