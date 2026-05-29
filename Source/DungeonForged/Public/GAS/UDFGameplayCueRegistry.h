// Source/DungeonForged/Public/GAS/UDFGameplayCueRegistry.h
#pragma once

#include "CoreMinimal.h"
#include "FX/UDFCombatFeedbackTypes.h"
#include "GAS/Elemental/UDFElementalReactionSubsystem.h"
#include "GameplayTagContainer.h"

struct FGameplayCueReferencePair;

/**
 * Maps @c Impact.* taxonomy tags to native @c GameplayCue.* notifies.
 * Used by @c DFGameplayCueRegistration and @c UDFCombatFeedbackLibrary.
 */
namespace UDFGameplayCueRegistry
{
	/** @c Impact.* / band tag -> @c GameplayCue.Combat.* (invalid if unmapped). */
	DUNGEONFORGED_API FGameplayTag ResolveGameplayCueForImpactTag(FGameplayTag ImpactTag);

	/** Band + optional @c Damage.Source.* -> impact tag, then gameplay cue tag. */
	DUNGEONFORGED_API FGameplayTag ResolveGameplayCueForHit(
		EDFHitFeedbackBand Band,
		FGameplayTag DamageSourceTag);

	/** Built-in / table-driven elemental reaction -> @c GameplayCue.Element.Reaction.* */
	DUNGEONFORGED_API FGameplayTag ResolveGameplayCueForReaction(EDFElementalRuntimeReaction Reaction);

	/** Authority: replicate reaction cue to clients via target ASC. */
	DUNGEONFORGED_API void ExecuteReactionCue(AActor* Target, AActor* Instigator, EDFElementalRuntimeReaction Reaction);

	DUNGEONFORGED_API void AppendNativeCombatCueReferences(TArray<FGameplayCueReferencePair>& OutRefs);
	DUNGEONFORGED_API void AppendNativeElementCueReferences(TArray<FGameplayCueReferencePair>& OutRefs);
}
