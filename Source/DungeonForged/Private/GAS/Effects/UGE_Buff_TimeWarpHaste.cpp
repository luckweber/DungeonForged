// Source/DungeonForged/Private/GAS/Effects/UGE_Buff_TimeWarpHaste.cpp
#include "GAS/Effects/UGE_Buff_TimeWarpHaste.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Buff_TimeWarpHaste::UGE_Buff_TimeWarpHaste()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(8.f));

	{
		FGameplayModifierInfo Mod;
		Mod.Attribute = UDFAttributeSet::GetCooldownReductionAttribute();
		Mod.ModifierOp = EGameplayModOp::Additive;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.5f));
		Modifiers.Add(Mod);
	}
	{
		FGameplayModifierInfo Mod;
		Mod.Attribute = UDFAttributeSet::GetSpellDamageAmpAttribute();
		Mod.ModifierOp = EGameplayModOp::Additive;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.3f));
		Modifiers.Add(Mod);
	}
}

void UGE_Buff_TimeWarpHaste::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Gr;
	Gr.AddTag(FDFGameplayTags::Buff_Mage_TimeWarpHaste);
	Grant.SetAndApplyTargetTagChanges(Gr);
}
