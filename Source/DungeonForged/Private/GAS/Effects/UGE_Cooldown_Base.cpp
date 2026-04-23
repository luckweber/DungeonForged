// Source/DungeonForged/Private/GAS/Effects/UGE_Cooldown_Base.cpp
#include "GAS/Effects/UGE_Cooldown_Base.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffect.h"

UGE_Cooldown_Base::UGE_Cooldown_Base()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	{
		FSetByCallerFloat Sbc;
		Sbc.DataTag = FDFGameplayTags::Data_Cooldown;
		DurationMagnitude = FGameplayEffectModifierMagnitude(Sbc);
	}
}
