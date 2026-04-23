// Source/DungeonForged/Public/GAS/UDFGEComponent_CancelAbilitiesOnApply.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "UDFGEComponent_CancelAbilitiesOnApply.generated.h"

/** Cancels active abilities whose ability tags match the root `Ability` tag (see FDFGameplayTags::Ability_Parent). Server only. */
UCLASS(DisplayName = "DF Cancel Active Abilities On Apply")
class DUNGEONFORGED_API UDFGEComponent_CancelAbilitiesOnApply : public UGameplayEffectComponent
{
	GENERATED_BODY()
public:
	virtual void OnGameplayEffectApplied(
		FActiveGameplayEffectsContainer& ActiveGEContainer,
		FGameplayEffectSpec& GESpec,
		FPredictionKey& PredictionKey) const override;
};
