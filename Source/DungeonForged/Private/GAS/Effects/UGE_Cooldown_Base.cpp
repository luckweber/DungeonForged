// Source/DungeonForged/Private/GAS/Effects/UGE_Cooldown_Base.cpp
#include "GAS/Effects/UGE_Cooldown_Base.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"

UGE_Cooldown_Base::UGE_Cooldown_Base()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	{
		FSetByCallerFloat Sbc;
		Sbc.DataTag = FDFGameplayTags::Data_Cooldown;
		DurationMagnitude = FGameplayEffectModifierMagnitude(Sbc);
	}
}

void UGE_Cooldown_Base::ConfigureEffectCDO()
{
	Super::ConfigureEffectCDO();
	UAssetTagsGameplayEffectComponent& AssetTags = FindOrAddComponent<UAssetTagsGameplayEffectComponent>();
	FInheritedTagContainer AssetTagChanges;
	AssetTagChanges.AddTag(FDFGameplayTags::Ability_Cooldown);
	AssetTags.SetAndApplyAssetTagChanges(AssetTagChanges);

	// FGameplayEffectQuery::MakeQuery_Match*EffectTags matches InheritableGameplayEffectTags, not asset-tag components alone.
	InheritableGameplayEffectTags.AddTag(FDFGameplayTags::Ability_Cooldown);
	if (CooldownAssociatedAbilityTag.IsValid())
	{
		InheritableGameplayEffectTags.AddTag(CooldownAssociatedAbilityTag);
	}
}
