// Source/DungeonForged/Private/GAS/Effects/UGE_ManaShield_Active.cpp
#include "GAS/Effects/UGE_ManaShield_Active.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_ManaShield_Active::UGE_ManaShield_Active()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
}

void UGE_ManaShield_Active::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Gr;
	Gr.AddTag(FDFGameplayTags::State_ManaShieldActive);
	Grant.SetAndApplyTargetTagChanges(Gr);
}
