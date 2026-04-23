// Source/DungeonForged/Private/GAS/Effects/UGE_Teleport_IFrame.cpp
#include "GAS/Effects/UGE_Teleport_IFrame.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Teleport_IFrame::UGE_Teleport_IFrame()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.3f));
}

void UGE_Teleport_IFrame::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Gr;
	Gr.AddTag(FDFGameplayTags::State_Invulnerable);
	Grant.SetAndApplyTargetTagChanges(Gr);
}
