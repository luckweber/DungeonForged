// Source/DungeonForged/Private/GAS/Effects/UDFMMC_BleedDoT.cpp
#include "GAS/Effects/UDFMMC_BleedDoT.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffectTypes.h"

float UDFMMC_BleedDoT::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTag Tag = FDFGameplayTags::Data_Damage.IsValid()
		? FDFGameplayTags::Data_Damage
		: FGameplayTag::RequestGameplayTag(FName("Data.Damage"), false);
	if (!Tag.IsValid())
	{
		return 0.f;
	}
	const float D = GetSetByCallerMagnitudeByTag(Spec, Tag);
	return -FMath::Max(0.f, D) * 0.2f;
}
