// Source/DungeonForged/Private/GAS/Effects/UGE_Rogue_ShadowStep_Stealth.cpp
#include "GAS/Effects/UGE_Rogue_ShadowStep_Stealth.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Rogue_ShadowStep_Stealth::UGE_Rogue_ShadowStep_Stealth()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.5f));
	FGameplayModifierInfo Mod;
	Mod.Attribute = UDFAttributeSet::GetMovementSpeedMultiplierAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.5f));
	Modifiers.Add(Mod);
}

void UGE_Rogue_ShadowStep_Stealth::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer C;
	C.AddTag(FDFGameplayTags::State_Invisible);
	Grant.SetAndApplyTargetTagChanges(C);
}
