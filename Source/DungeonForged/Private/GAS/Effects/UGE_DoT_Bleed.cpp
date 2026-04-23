// Source/DungeonForged/Private/GAS/Effects/UGE_DoT_Bleed.cpp
#include "GAS/Effects/UGE_DoT_Bleed.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UDFMMC_BleedDoT.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_DoT_Bleed::UGE_DoT_Bleed()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	{
		FSetByCallerFloat Sbc;
		Sbc.DataTag = FDFGameplayTags::Data_Duration;
		DurationMagnitude = FGameplayEffectModifierMagnitude(Sbc);
	}
	Period = FScalableFloat(0.5f);

	FGameplayModifierInfo Mod;
	Mod.Attribute = UDFAttributeSet::GetHealthAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	FCustomCalculationBasedFloat Cc;
	Cc.CalculationClassMagnitude = UDFMMC_BleedDoT::StaticClass();
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(Cc);
	Modifiers.Add(Mod);
}

void UGE_DoT_Bleed::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Gr;
	Gr.AddTag(FDFGameplayTags::Effect_DoT_Bleed);
	Grant.SetAndApplyTargetTagChanges(Gr);
}
