// Source/DungeonForged/Private/GAS/Effects/UGE_Buff_BattleHymn.cpp
#include "GAS/Effects/UGE_Buff_BattleHymn.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Buff_BattleHymn::UGE_Buff_BattleHymn()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(12.f));

	auto AddMod = [this](const FGameplayAttribute& Attr, const float Mag)
	{
		FGameplayModifierInfo Mod;
		Mod.Attribute = Attr;
		Mod.ModifierOp = EGameplayModOp::Additive;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Mag));
		Modifiers.Add(Mod);
	};
	AddMod(UDFAttributeSet::GetStrengthAttribute(), 15.f);
	AddMod(UDFAttributeSet::GetIntelligenceAttribute(), 15.f);
	AddMod(UDFAttributeSet::GetAgilityAttribute(), 15.f);
	AddMod(UDFAttributeSet::GetCooldownReductionAttribute(), 0.2f);
	AddMod(UDFAttributeSet::GetCritChanceAttribute(), 0.1f);
}

void UGE_Buff_BattleHymn::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Gr;
	Gr.AddTag(FDFGameplayTags::Effect_Buff_BattleHymn);
	Grant.SetAndApplyTargetTagChanges(Gr);
}
