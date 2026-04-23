// Source/DungeonForged/Private/GAS/UDFTrueDamageExecution.cpp
#include "GAS/UDFTrueDamageExecution.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"

UDFTrueDamageExecution::UDFTrueDamageExecution()
	: UGameplayEffectExecutionCalculation()
{
}

void UDFTrueDamageExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	if (!ExecutionParams.GetTargetAbilitySystemComponent())
	{
		return;
	}
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const FGameplayTag DataDamageTag = FDFGameplayTags::Data_Damage.IsValid()
		? FDFGameplayTags::Data_Damage
		: FGameplayTag::RequestGameplayTag(FName("Data.Damage"), false);
	const float Base = DataDamageTag.IsValid()
		? Spec.GetSetByCallerMagnitude(DataDamageTag, false, 0.f)
		: 0.f;
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UDFAttributeSet::GetHealthAttribute(),
		EGameplayModOp::Additive, -FMath::Max(0.f, Base)));
}
