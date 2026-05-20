// Source/DungeonForged/Private/GAS/UDFGEComponent_StatusResistDuration.cpp
#include "GAS/UDFGEComponent_StatusResistDuration.h"

#include "AbilitySystemComponent.h"
#include "DFAssetManager.h"
#include "Data/UDFCombatTuningData.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"

void UDFGEComponent_StatusResistDuration::OnGameplayEffectApplied(
	FActiveGameplayEffectsContainer& ActiveGEContainer,
	FGameplayEffectSpec& GESpec,
	FPredictionKey& PredictionKey) const
{
	(void)PredictionKey;
	if (!GESpec.Def || !FDFGameplayTags::Effect_CrowdControl.IsValid())
	{
		return;
	}
	const FGameplayTagContainer& AssetTags = GESpec.GetDynamicAssetTags();
	if (!AssetTags.HasTag(FDFGameplayTags::Effect_CrowdControl))
	{
		return;
	}
	UAbilitySystemComponent* const ASC = ActiveGEContainer.Owner;
	if (!ASC)
	{
		return;
	}
	if (FDFGameplayTags::State_CCIgnore.IsValid() && ASC->HasMatchingGameplayTag(FDFGameplayTags::State_CCIgnore))
	{
		return;
	}
	const float Resist = FMath::Max(0.f, ASC->GetNumericAttribute(UDFAttributeSet::GetStatusResistAttribute()));
	if (Resist <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	float MinScale = 0.15f;
	if (const UDFCombatTuningData* const Tuning = UDFAssetManager::Get().GetCombatTuningData())
	{
		MinScale = Tuning->StatusResistMinDurationScale;
	}
	const float Scale = FMath::Clamp(1.f - Resist, MinScale, 1.f);
	if (Scale >= 1.f - KINDA_SMALL_NUMBER)
	{
		return;
	}
	if (GESpec.Def->DurationPolicy != EGameplayEffectDurationType::HasDuration)
	{
		return;
	}
	float Duration = GESpec.GetDuration();
	if (FDFGameplayTags::Data_Duration.IsValid())
	{
		const float Sbc = GESpec.GetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, false, -1.f);
		if (Sbc > KINDA_SMALL_NUMBER)
		{
			Duration = Sbc;
		}
	}
	if (Duration <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	const float NewDuration = Duration * Scale;
	GESpec.SetDuration(NewDuration, true);
	if (FDFGameplayTags::Data_Duration.IsValid())
	{
		GESpec.SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, NewDuration);
	}
}
