// Source/DungeonForged/Private/GAS/Effects/UGE_Death.cpp
#include "GAS/Effects/UGE_Death.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Death::UGE_Death()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Mod;
	Mod.Attribute = UDFAttributeSet::GetHealthAttribute();
	Mod.ModifierOp = EGameplayModOp::Override;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.f));
	Modifiers.Add(Mod);
}

void UGE_Death::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Gr;
	Gr.AddTag(FDFGameplayTags::State_Dead);
	Grant.SetAndApplyTargetTagChanges(Gr);
}
