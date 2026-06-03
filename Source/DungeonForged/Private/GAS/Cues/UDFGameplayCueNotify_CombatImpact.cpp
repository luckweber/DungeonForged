// Source/DungeonForged/Private/GAS/Cues/UDFGameplayCueNotify_CombatImpact.cpp
#include "GAS/Cues/UDFGameplayCueNotify_CombatImpact.h"

#include "FX/UDFCombatFeedbackLibrary.h"
#include "GAS/DFGameplayTags.h"

namespace
{
FGameplayTag ImpactTagFromGameplayCueTag(const FGameplayTag CueTag)
{
	if (!CueTag.IsValid())
	{
		return FGameplayTag();
	}
	if (CueTag == FDFGameplayTags::GameplayCue_Combat_Impact_Knockback)
	{
		return FDFGameplayTags::Impact_Knockback;
	}
	if (CueTag == FDFGameplayTags::GameplayCue_Combat_Impact_Critical)
	{
		return FDFGameplayTags::Impact_Critical;
	}
	if (CueTag == FDFGameplayTags::GameplayCue_Combat_Impact_Heavy)
	{
		return FDFGameplayTags::Impact_Heavy;
	}
	if (CueTag == FDFGameplayTags::GameplayCue_Combat_Impact_Light)
	{
		return FDFGameplayTags::Impact_Light;
	}
	if (CueTag == FDFGameplayTags::GameplayCue_Combat_Block)
	{
		return FDFGameplayTags::Impact_Light_Blunt;
	}
	if (CueTag == FDFGameplayTags::GameplayCue_Combat_Parry)
	{
		return FDFGameplayTags::Impact_Heavy_Slash;
	}
	return FGameplayTag();
}

bool IsImpactFamilyTag(const FGameplayTag& Tag)
{
	return Tag.MatchesTag(FDFGameplayTags::Impact_Light)
		|| Tag.MatchesTag(FDFGameplayTags::Impact_Heavy)
		|| Tag.MatchesTag(FDFGameplayTags::Impact_Critical)
		|| Tag.MatchesTag(FDFGameplayTags::Impact_Knockback);
}
} // namespace

UDFGameplayCueNotify_CombatImpact::UDFGameplayCueNotify_CombatImpact() = default;

bool UDFGameplayCueNotify_CombatImpact::OnExecute_Implementation(
	AActor* const Target, const FGameplayCueParameters& Parameters) const
{
	if (!Target || Target->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	FGameplayTag ImpactTag;
	for (const FGameplayTag& Tag : Parameters.AggregatedSourceTags)
	{
		if (IsImpactFamilyTag(Tag))
		{
			ImpactTag = Tag;
			break;
		}
	}
	if (!ImpactTag.IsValid())
	{
		const FGameplayTag CueTag = Parameters.MatchedTagName.IsValid() ? Parameters.MatchedTagName : GameplayCueTag;
		ImpactTag = ImpactTagFromGameplayCueTag(CueTag);
	}
	if (!ImpactTag.IsValid())
	{
		return false;
	}

	const FVector Loc = Parameters.Location.IsNearlyZero()
		? Target->GetActorLocation()
		: FVector(Parameters.Location);
	const FVector Normal = Parameters.Normal.IsNearlyZero() ? FVector::UpVector : FVector(Parameters.Normal);
	UDFCombatFeedbackLibrary::SpawnImpactAssetsForTag(Target, Loc, Normal, ImpactTag);
	return true;
}
