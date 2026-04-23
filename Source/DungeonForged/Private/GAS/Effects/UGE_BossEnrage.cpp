// Source/DungeonForged/Private/GAS/Effects/UGE_BossEnrage.cpp
#include "GAS/Effects/UGE_BossEnrage.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_BossEnrage::UGE_BossEnrage()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	{
		FGameplayModifierInfo ModS;
		ModS.Attribute = UDFAttributeSet::GetStrengthAttribute();
		ModS.ModifierOp = EGameplayModOp::Additive;
		ModS.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(50.f));
		Modifiers.Add(ModS);
	}
	{
		FGameplayModifierInfo ModM;
		ModM.Attribute = UDFAttributeSet::GetMovementSpeedMultiplierAttribute();
		ModM.ModifierOp = EGameplayModOp::Additive;
		ModM.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.3f));
		Modifiers.Add(ModM);
	}
}

void UGE_BossEnrage::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& T = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer C;
	C.AddTag(FDFGameplayTags::State_CCIgnore);
	C.AddTag(FDFGameplayTags::State_BossEnraged);
	T.SetAndApplyTargetTagChanges(C);
}
