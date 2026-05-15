// Source/DungeonForged/Private/GAS/Effects/UGE_Cooldown_Base.cpp
#include "GAS/Effects/UGE_Cooldown_Base.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/BlockAbilityTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

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
	if (CooldownAssociatedAbilityTag.IsValid())
	{
		AssetTagChanges.AddTag(CooldownAssociatedAbilityTag);
	}
	AssetTags.SetAndApplyAssetTagChanges(AssetTagChanges);

	// UGameplayAbility::GetCooldownTags() reads GetGrantedTags() on the cooldown CDO.
	// CheckCooldown then calls ASC->HasAnyMatchingGameplayTags — asset tags alone do not block activation.
	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer GrantedTags;
	GrantedTags.AddTag(FDFGameplayTags::Ability_Cooldown);
	if (CooldownAssociatedAbilityTag.IsValid())
	{
		GrantedTags.AddTag(CooldownAssociatedAbilityTag);
	}
	Grant.SetAndApplyTargetTagChanges(GrantedTags);

	if (CooldownAssociatedAbilityTag.IsValid())
	{
		UBlockAbilityTagsGameplayEffectComponent& Block = FindOrAddComponent<UBlockAbilityTagsGameplayEffectComponent>();
		FInheritedTagContainer BlockedAbilityTags;
		BlockedAbilityTags.AddTag(CooldownAssociatedAbilityTag);
		Block.SetAndApplyBlockedAbilityTagChanges(BlockedAbilityTags);
	}
}
