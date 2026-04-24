// Source/DungeonForged/Private/GAS/Effects/UGE_Boss_VoidBarrier.cpp
#include "GAS/Effects/UGE_Boss_VoidBarrier.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Boss_VoidBarrier::UGE_Boss_VoidBarrier()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	{
		FSetByCallerFloat Sbc;
		Sbc.DataTag = FDFGameplayTags::Data_Duration;
		DurationMagnitude = FGameplayEffectModifierMagnitude(Sbc);
	}
	FGameplayModifierInfo Mod;
	Mod.Attribute = UDFAttributeSet::GetArmorAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(99999.f));
	Modifiers.Add(Mod);
}

void UGE_Boss_VoidBarrier::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& G = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer T;
	T.AddTag(FDFGameplayTags::State_Invulnerable);
	G.SetAndApplyTargetTagChanges(T);
}
