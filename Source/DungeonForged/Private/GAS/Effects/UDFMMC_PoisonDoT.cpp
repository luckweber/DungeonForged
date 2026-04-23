// Source/DungeonForged/Private/GAS/Effects/UDFMMC_PoisonDoT.cpp
#include "GAS/Effects/UDFMMC_PoisonDoT.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffectTypes.h"

float UDFMMC_PoisonDoT::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTag Tag = FDFGameplayTags::Data_Damage.IsValid()
		? FDFGameplayTags::Data_Damage
		: FGameplayTag::RequestGameplayTag(FName("Data.Damage"), false);
	const float FromCaller = Tag.IsValid() ? GetSetByCallerMagnitudeByTag(Spec, Tag) : 0.f;
	if (FromCaller > 0.f)
	{
		return -FMath::Max(5.f, FromCaller);
	}
	return -5.f;
}
