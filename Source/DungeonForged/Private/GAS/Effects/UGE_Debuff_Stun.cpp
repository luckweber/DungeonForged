// Source/DungeonForged/Private/GAS/Effects/UGE_Debuff_Stun.cpp
#include "GAS/Effects/UGE_Debuff_Stun.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFGEComponent_CancelAbilitiesOnApply.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Debuff_Stun::UGE_Debuff_Stun()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	{
		FSetByCallerFloat Sbc;
		Sbc.DataTag = FDFGameplayTags::Data_Duration;
		DurationMagnitude = FGameplayEffectModifierMagnitude(Sbc);
	}
}

void UGE_Debuff_Stun::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Gr;
	Gr.AddTag(FDFGameplayTags::State_Stunned);
	Grant.SetAndApplyTargetTagChanges(Gr);
	FindOrAddComponent<UDFGEComponent_CancelAbilitiesOnApply>();
	// Enrage / Boss: do not apply stun if target has State.CCIgnore
	UTargetTagRequirementsGameplayEffectComponent& AppReq = FindOrAddComponent<UTargetTagRequirementsGameplayEffectComponent>();
	FGameplayTagRequirements StunAppReqs;
	StunAppReqs.IgnoreTags.AddTag(FDFGameplayTags::State_CCIgnore);
	AppReq.ApplicationTagRequirements = StunAppReqs;
}
