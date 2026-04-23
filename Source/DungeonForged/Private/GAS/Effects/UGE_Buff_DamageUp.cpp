// Source/DungeonForged/Private/GAS/Effects/UGE_Buff_DamageUp.cpp
#include "GAS/Effects/UGE_Buff_DamageUp.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Buff_DamageUp::UGE_Buff_DamageUp()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	{
		FSetByCallerFloat Sbc;
		Sbc.DataTag = FDFGameplayTags::Data_Duration;
		DurationMagnitude = FGameplayEffectModifierMagnitude(Sbc);
	}

	FGameplayModifierInfo Mod;
	Mod.Attribute = UDFAttributeSet::GetStrengthAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	{
		FSetByCallerFloat Sbc;
		Sbc.DataTag = FDFGameplayTags::Data_Magnitude;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(Sbc);
	}
	Modifiers.Add(Mod);
}

void UGE_Buff_DamageUp::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Gr;
	Gr.AddTag(FDFGameplayTags::Effect_Buff_DamageUp);
	Grant.SetAndApplyTargetTagChanges(Gr);
}
