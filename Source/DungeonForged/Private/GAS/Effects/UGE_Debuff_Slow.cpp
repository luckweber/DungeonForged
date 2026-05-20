// Source/DungeonForged/Private/GAS/Effects/UGE_Debuff_Slow.cpp
#include "GAS/Effects/UGE_Debuff_Slow.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UDFMMC_SlowMagnitude.h"
#include "GAS/UDFAttributeSet.h"
#include "GAS/UDFGEComponent_StatusResistDuration.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Debuff_Slow::UGE_Debuff_Slow()
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
	FCustomCalculationBasedFloat Cc;
	Cc.CalculationClassMagnitude = UDFMMC_SlowMagnitude::StaticClass();
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(Cc);
	Modifiers.Add(Mod);
}

void UGE_Debuff_Slow::ConfigureEffectCDO()
{
	UAssetTagsGameplayEffectComponent& AssetTags = FindOrAddComponent<UAssetTagsGameplayEffectComponent>();
	FInheritedTagContainer AssetTagChanges;
	AssetTagChanges.AddTag(FDFGameplayTags::Effect_CrowdControl);
	AssetTags.SetAndApplyAssetTagChanges(AssetTagChanges);

	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Gr;
	Gr.AddTag(FDFGameplayTags::Effect_Debuff_Slow);
	Grant.SetAndApplyTargetTagChanges(Gr);
	FindOrAddComponent<UDFGEComponent_StatusResistDuration>();
}
