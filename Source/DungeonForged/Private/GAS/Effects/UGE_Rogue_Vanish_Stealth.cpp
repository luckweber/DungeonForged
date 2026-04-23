// Source/DungeonForged/Private/GAS/Effects/UGE_Rogue_Vanish_Stealth.cpp
#include "GAS/Effects/UGE_Rogue_Vanish_Stealth.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Rogue_Vanish_Stealth::UGE_Rogue_Vanish_Stealth()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	{
		FSetByCallerFloat Sbc;
		Sbc.DataTag = FDFGameplayTags::Data_Duration;
		DurationMagnitude = FGameplayEffectModifierMagnitude(Sbc);
	}
	FGameplayModifierInfo Mod;
	Mod.Attribute = UDFAttributeSet::GetMovementSpeedMultiplierAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.2f));
	Modifiers.Add(Mod);
}

void UGE_Rogue_Vanish_Stealth::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer C;
	C.AddTag(FDFGameplayTags::State_Invisible);
	C.AddTag(FDFGameplayTags::State_Stealthed);
	Grant.SetAndApplyTargetTagChanges(C);
}
