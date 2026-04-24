// Source/DungeonForged/Private/GAS/Effects/UGE_LevelUp_StatScaling.cpp

#include "GAS/Effects/UGE_LevelUp_StatScaling.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_LevelUp_StatScaling::UGE_LevelUp_StatScaling()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
}

void UGE_LevelUp_StatScaling::ConfigureEffectCDO()
{
	Super::ConfigureEffectCDO();
	if (FDFGameplayTags::Effect_Buff_LevelStatScaling.IsValid())
	{
		UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
		FInheritedTagContainer Gr;
		Gr.AddTag(FDFGameplayTags::Effect_Buff_LevelStatScaling);
		Grant.SetAndApplyTargetTagChanges(Gr);
	}

	auto AddCaller = [this](const FGameplayAttribute& Attr, const FGameplayTag& DataTag) {
		FGameplayModifierInfo Mod;
		Mod.Attribute = Attr;
		Mod.ModifierOp = EGameplayModOp::Additive;
		FSetByCallerFloat Sbc;
		Sbc.DataTag = DataTag;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(Sbc);
		Modifiers.Add(Mod);
	};

	if (FDFGameplayTags::Data_LevelUp_MaxHealthAdd.IsValid())
	{
		AddCaller(UDFAttributeSet::GetMaxHealthAttribute(), FDFGameplayTags::Data_LevelUp_MaxHealthAdd);
	}
	if (FDFGameplayTags::Data_LevelUp_MaxManaAdd.IsValid())
	{
		AddCaller(UDFAttributeSet::GetMaxManaAttribute(), FDFGameplayTags::Data_LevelUp_MaxManaAdd);
	}
	if (FDFGameplayTags::Data_LevelUp_StrengthAdd.IsValid())
	{
		AddCaller(UDFAttributeSet::GetStrengthAttribute(), FDFGameplayTags::Data_LevelUp_StrengthAdd);
	}
	if (FDFGameplayTags::Data_LevelUp_IntelligenceAdd.IsValid())
	{
		AddCaller(UDFAttributeSet::GetIntelligenceAttribute(), FDFGameplayTags::Data_LevelUp_IntelligenceAdd);
	}
	if (FDFGameplayTags::Data_LevelUp_AgilityAdd.IsValid())
	{
		AddCaller(UDFAttributeSet::GetAgilityAttribute(), FDFGameplayTags::Data_LevelUp_AgilityAdd);
	}
}
