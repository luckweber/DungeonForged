// Source/DungeonForged/Private/GAS/Effects/UGE_DoT_Poison.cpp
#include "GAS/Effects/UGE_DoT_Poison.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GAS/Effects/UDFMMC_PoisonDoT.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_DoT_Poison::UGE_DoT_Poison()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	{
		FSetByCallerFloat Sbc;
		Sbc.DataTag = FDFGameplayTags::Data_Duration;
		DurationMagnitude = FGameplayEffectModifierMagnitude(Sbc);
	}
	Period = FScalableFloat(1.f);

	StackingType = EGameplayEffectStackingType::AggregateBySource;
	StackLimitCount = 12;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

	FGameplayModifierInfo Mod;
	Mod.Attribute = UDFAttributeSet::GetHealthAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	FCustomCalculationBasedFloat Cc;
	Cc.CalculationClassMagnitude = UDFMMC_PoisonDoT::StaticClass();
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(Cc);
	Modifiers.Add(Mod);
}

void UGE_DoT_Poison::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Gr;
	Gr.AddTag(FDFGameplayTags::Effect_DoT_Poison);
	Grant.SetAndApplyTargetTagChanges(Gr);
}
