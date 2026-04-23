// Source/DungeonForged/Private/GAS/Effects/UGE_Damage_True.cpp
#include "GAS/Effects/UGE_Damage_True.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFTrueDamageExecution.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"

UGE_Damage_True::UGE_Damage_True()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UDFTrueDamageExecution::StaticClass();
	Executions.Add(ExecDef);
}

void UGE_Damage_True::ConfigureEffectCDO()
{
	UAssetTagsGameplayEffectComponent& AssetTags = FindOrAddComponent<UAssetTagsGameplayEffectComponent>();
	FInheritedTagContainer T;
	T.AddTag(FDFGameplayTags::Effect_Damage_True);
	AssetTags.SetAndApplyAssetTagChanges(T);
}
