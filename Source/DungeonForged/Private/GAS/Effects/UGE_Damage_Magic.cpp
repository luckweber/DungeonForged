// Source/DungeonForged/Private/GAS/Effects/UGE_Damage_Magic.cpp
#include "GAS/Effects/UGE_Damage_Magic.h"
#include "GAS/DFDamageCalculation.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"

UGE_Damage_Magic::UGE_Damage_Magic()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UDFDamageCalculation::StaticClass();
	Executions.Add(ExecDef);
}

void UGE_Damage_Magic::ConfigureEffectCDO()
{
	UAssetTagsGameplayEffectComponent& AssetTags = FindOrAddComponent<UAssetTagsGameplayEffectComponent>();
	FInheritedTagContainer T;
	T.AddTag(FDFGameplayTags::Effect_Damage_Magic);
	AssetTags.SetAndApplyAssetTagChanges(T);
}
