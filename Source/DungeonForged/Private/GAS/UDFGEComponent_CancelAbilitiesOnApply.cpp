// Source/DungeonForged/Private/GAS/UDFGEComponent_CancelAbilitiesOnApply.cpp
#include "GAS/UDFGEComponent_CancelAbilitiesOnApply.h"
#include "GAS/DFGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"

void UDFGEComponent_CancelAbilitiesOnApply::OnGameplayEffectApplied(
	FActiveGameplayEffectsContainer& ActiveGEContainer,
	FGameplayEffectSpec& GESpec,
	FPredictionKey& PredictionKey) const
{
	(void)GESpec;
	(void)PredictionKey;
	if (!FDFGameplayTags::Ability_Parent.IsValid())
	{
		return;
	}
	UAbilitySystemComponent* const ASC = ActiveGEContainer.Owner;
	if (!ASC)
	{
		return;
	}
	if (UObject* const O = ASC->GetOwner())
	{
		if (const AActor* const A = Cast<AActor>(O))
		{
			if (!A->HasAuthority())
			{
				return;
			}
		}
	}
	else
	{
		return;
	}
	FGameplayTagContainer WithTags;
	WithTags.AddTag(FDFGameplayTags::Ability_Parent);
	ASC->CancelAbilities(&WithTags, nullptr, nullptr);
}
