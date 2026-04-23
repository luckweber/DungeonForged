// Source/DungeonForged/Private/GAS/Effects/UGE_Damage_Physical.cpp
#include "GAS/Effects/UGE_Damage_Physical.h"
#include "GAS/DFDamageCalculation.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"

UGE_Damage_Physical::UGE_Damage_Physical()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UDFDamageCalculation::StaticClass();
	Executions.Add(ExecDef);
}

void UGE_Damage_Physical::ConfigureEffectCDO()
{
	UAssetTagsGameplayEffectComponent& AssetTags = FindOrAddComponent<UAssetTagsGameplayEffectComponent>();
	FInheritedTagContainer Tags;
	Tags.AddTag(FDFGameplayTags::Effect_Damage_Physical);
	AssetTags.SetAndApplyAssetTagChanges(Tags);
}
