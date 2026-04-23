// Source/DungeonForged/Private/GAS/Effects/UGE_Buff_Rogue_KillingSpree.cpp
#include "GAS/Effects/UGE_Buff_Rogue_KillingSpree.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Buff_Rogue_KillingSpree::UGE_Buff_Rogue_KillingSpree()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	{
		FSetByCallerFloat Sbc;
		Sbc.DataTag = FDFGameplayTags::Data_Duration;
		DurationMagnitude = FGameplayEffectModifierMagnitude(Sbc);
	}
	{
		FGameplayModifierInfo A;
		A.Attribute = UDFAttributeSet::GetAgilityAttribute();
		A.ModifierOp = EGameplayModOp::Additive;
		FSetByCallerFloat Sb;
		Sb.DataTag = FDFGameplayTags::Data_Magnitude;
		A.ModifierMagnitude = FGameplayEffectModifierMagnitude(Sb);
		Modifiers.Add(A);
	}
	{
		FGameplayModifierInfo M;
		M.Attribute = UDFAttributeSet::GetMovementSpeedMultiplierAttribute();
		M.ModifierOp = EGameplayModOp::Additive;
		M.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.2f));
		Modifiers.Add(M);
	}
}

void UGE_Buff_Rogue_KillingSpree::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer T;
	T.AddTag(FDFGameplayTags::Buff_Rogue_KillingSpree);
	Grant.SetAndApplyTargetTagChanges(T);
}
