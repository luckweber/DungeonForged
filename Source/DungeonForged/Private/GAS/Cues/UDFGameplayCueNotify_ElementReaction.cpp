// Source/DungeonForged/Private/GAS/Cues/UDFGameplayCueNotify_ElementReaction.cpp
#include "GAS/Cues/UDFGameplayCueNotify_ElementReaction.h"

#include "GAS/DFGameplayTags.h"
#include "GAS/Elemental/UDFElementalReactionSubsystem.h"
#include "Engine/World.h"

namespace
{
EDFElementalRuntimeReaction ReactionFromCueTag(const FGameplayTag CueTag)
{
	if (CueTag == FDFGameplayTags::GameplayCue_Element_Reaction_Melt)
	{
		return EDFElementalRuntimeReaction::Melt;
	}
	if (CueTag == FDFGameplayTags::GameplayCue_Element_Reaction_Electrocute)
	{
		return EDFElementalRuntimeReaction::Electrocute;
	}
	if (CueTag == FDFGameplayTags::GameplayCue_Element_Reaction_Steam)
	{
		return EDFElementalRuntimeReaction::Steam;
	}
	if (CueTag == FDFGameplayTags::GameplayCue_Element_Reaction_Generic)
	{
		return EDFElementalRuntimeReaction::TableDriven;
	}
	return EDFElementalRuntimeReaction::None;
}
} // namespace

UDFGameplayCueNotify_ElementReaction::UDFGameplayCueNotify_ElementReaction() = default;

bool UDFGameplayCueNotify_ElementReaction::OnExecute_Implementation(
	AActor* const Target, const FGameplayCueParameters& Parameters) const
{
	if (!Target || Target->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}
	const FGameplayTag CueTag = Parameters.MatchedTagName.IsValid() ? Parameters.MatchedTagName : GameplayCueTag;
	const EDFElementalRuntimeReaction Reaction = ReactionFromCueTag(CueTag);
	if (Reaction == EDFElementalRuntimeReaction::None)
	{
		return false;
	}
	if (UWorld* const W = Target->GetWorld())
	{
		if (UDFElementalReactionSubsystem* const Sub = W->GetSubsystem<UDFElementalReactionSubsystem>())
		{
			Sub->TrySpawnReactionVFX(Reaction, Target);
			return true;
		}
	}
	return false;
}
