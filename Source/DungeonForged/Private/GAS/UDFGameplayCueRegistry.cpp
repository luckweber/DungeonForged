// Source/DungeonForged/Private/GAS/UDFGameplayCueRegistry.cpp
#include "GAS/UDFGameplayCueRegistry.h"

#include "FX/UDFCombatFeedbackLibrary.h"
#include "GAS/Cues/UDFGameplayCueNotify_CombatImpact.h"
#include "GAS/Cues/UDFGameplayCueNotify_ElementReaction.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayCueSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

namespace UDFGameplayCueRegistry
{
FGameplayTag ResolveGameplayCueForImpactTag(const FGameplayTag ImpactTag)
{
	if (!ImpactTag.IsValid())
	{
		return FGameplayTag();
	}
	if (ImpactTag.MatchesTag(FDFGameplayTags::Impact_Knockback))
	{
		return FDFGameplayTags::GameplayCue_Combat_Impact_Knockback;
	}
	if (ImpactTag.MatchesTag(FDFGameplayTags::Impact_Critical))
	{
		return FDFGameplayTags::GameplayCue_Combat_Impact_Critical;
	}
	if (ImpactTag.MatchesTag(FDFGameplayTags::Impact_Heavy))
	{
		return FDFGameplayTags::GameplayCue_Combat_Impact_Heavy;
	}
	if (ImpactTag.MatchesTag(FDFGameplayTags::Impact_Light))
	{
		return FDFGameplayTags::GameplayCue_Combat_Impact_Light;
	}
	return FGameplayTag();
}

FGameplayTag ResolveGameplayCueForHit(const EDFHitFeedbackBand Band, const FGameplayTag DamageSourceTag)
{
	const FGameplayTag ImpactTag = UDFCombatFeedbackLibrary::ResolveImpactTag(Band, DamageSourceTag);
	return ResolveGameplayCueForImpactTag(ImpactTag);
}

FGameplayTag ResolveGameplayCueForReaction(const EDFElementalRuntimeReaction Reaction)
{
	switch (Reaction)
	{
	case EDFElementalRuntimeReaction::Melt:
		return FDFGameplayTags::GameplayCue_Element_Reaction_Melt;
	case EDFElementalRuntimeReaction::Electrocute:
		return FDFGameplayTags::GameplayCue_Element_Reaction_Electrocute;
	case EDFElementalRuntimeReaction::Steam:
		return FDFGameplayTags::GameplayCue_Element_Reaction_Steam;
	case EDFElementalRuntimeReaction::TableDriven:
		return FDFGameplayTags::GameplayCue_Element_Reaction_Generic;
	default:
		return FGameplayTag();
	}
}

void ExecuteReactionCue(AActor* const Target, AActor* const Instigator, const EDFElementalRuntimeReaction Reaction)
{
	if (!Target || !Target->HasAuthority() || Reaction == EDFElementalRuntimeReaction::None)
	{
		return;
	}
	const FGameplayTag CueTag = ResolveGameplayCueForReaction(Reaction);
	if (!CueTag.IsValid())
	{
		return;
	}
	UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!ASC)
	{
		return;
	}
	FGameplayCueParameters Params;
	Params.Instigator = Instigator;
	Params.EffectCauser = Instigator ? Instigator : Target;
	Params.Location = Target->GetActorLocation();
	ASC->ExecuteGameplayCue(CueTag, Params);
}

void AppendNativeCombatCueReferences(TArray<FGameplayCueReferencePair>& OutRefs)
{
	const TSubclassOf<UGameplayCueNotify_Static> CueClass = UDFGameplayCueNotify_CombatImpact::StaticClass();
	auto Add = [&OutRefs, CueClass](const FGameplayTag& Tag)
	{
		if (Tag.IsValid())
		{
			OutRefs.Emplace(Tag, FSoftObjectPath(CueClass));
		}
	};
	Add(FDFGameplayTags::GameplayCue_Combat_Impact_Light);
	Add(FDFGameplayTags::GameplayCue_Combat_Impact_Heavy);
	Add(FDFGameplayTags::GameplayCue_Combat_Impact_Critical);
	Add(FDFGameplayTags::GameplayCue_Combat_Impact_Knockback);
	Add(FDFGameplayTags::GameplayCue_Combat_Block);
	Add(FDFGameplayTags::GameplayCue_Combat_Parry);
}

void AppendNativeElementCueReferences(TArray<FGameplayCueReferencePair>& OutRefs)
{
	const TSubclassOf<UGameplayCueNotify_Static> CueClass = UDFGameplayCueNotify_ElementReaction::StaticClass();
	auto Add = [&OutRefs, CueClass](const FGameplayTag& Tag)
	{
		if (Tag.IsValid())
		{
			OutRefs.Emplace(Tag, FSoftObjectPath(CueClass));
		}
	};
	Add(FDFGameplayTags::GameplayCue_Element_Reaction_Melt);
	Add(FDFGameplayTags::GameplayCue_Element_Reaction_Steam);
	Add(FDFGameplayTags::GameplayCue_Element_Reaction_Electrocute);
	Add(FDFGameplayTags::GameplayCue_Element_Reaction_Generic);
}
} // namespace UDFGameplayCueRegistry
