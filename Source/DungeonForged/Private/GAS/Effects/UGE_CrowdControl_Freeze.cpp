// Source/DungeonForged/Private/GAS/Effects/UGE_CrowdControl_Freeze.cpp
#include "GAS/Effects/UGE_CrowdControl_Freeze.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GAS/UDFGEComponent_StatusResistDuration.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_CrowdControl_Freeze::UGE_CrowdControl_Freeze()
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
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-1.f));
	Modifiers.Add(Mod);
}

void UGE_CrowdControl_Freeze::ConfigureEffectCDO()
{
	UAssetTagsGameplayEffectComponent& AssetTags = FindOrAddComponent<UAssetTagsGameplayEffectComponent>();
	FInheritedTagContainer AssetTagChanges;
	AssetTagChanges.AddTag(FDFGameplayTags::Effect_CrowdControl);
	AssetTags.SetAndApplyAssetTagChanges(AssetTagChanges);

	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Gr;
	Gr.AddTag(FDFGameplayTags::State_Rooted);
	Grant.SetAndApplyTargetTagChanges(Gr);
	FindOrAddComponent<UDFGEComponent_StatusResistDuration>();
}
