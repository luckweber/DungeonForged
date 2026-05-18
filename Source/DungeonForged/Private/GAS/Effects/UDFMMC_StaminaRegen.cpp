// Source/DungeonForged/Private/GAS/Effects/UDFMMC_StaminaRegen.cpp
#include "GAS/Effects/UDFMMC_StaminaRegen.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Data/UDFCombatTuningData.h"
#include "DFAssetManager.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffectTypes.h"

UDFMMC_StaminaRegen::UDFMMC_StaminaRegen()
{
}

float UDFMMC_StaminaRegen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const float Period = FMath::Max(Spec.GetPeriod(), 0.05f);
	const UDFCombatTuningData* const Tuning = UDFAssetManager::Get().GetCombatTuningData();
	float RatePerSec = Tuning ? Tuning->StaminaRegenOutOfCombat : 25.f;

	AActor* TargetActor = Spec.GetContext().GetEffectCauser();
	if (!TargetActor)
	{
		TargetActor = Spec.GetContext().GetInstigator();
	}
	if (UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
	{
		if (FDFGameplayTags::State_InCombat.IsValid() && ASC->HasMatchingGameplayTag(FDFGameplayTags::State_InCombat))
		{
			RatePerSec = Tuning ? Tuning->StaminaRegenInCombat : 8.f;
		}
	}
	return RatePerSec * Period;
}
