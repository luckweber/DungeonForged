// Source/DungeonForged/Private/GAS/DFDamageCalculation.cpp
#include "GAS/DFDamageCalculation.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"

UDFDamageCalculation::UDFDamageCalculation()
	: UGameplayEffectExecutionCalculation()
	, IntelligenceCapture(UDFAttributeSet::GetIntelligenceAttribute(), EGameplayEffectAttributeCaptureSource::Source, true)
	, MagicResistCapture(UDFAttributeSet::GetMagicResistAttribute(), EGameplayEffectAttributeCaptureSource::Target, true)
	, ArmorCapture(UDFAttributeSet::GetArmorAttribute(), EGameplayEffectAttributeCaptureSource::Target, true)
	, CritChanceCapture(UDFAttributeSet::GetCritChanceAttribute(), EGameplayEffectAttributeCaptureSource::Source, true)
	, CritMultCapture(UDFAttributeSet::GetCritMultiplierAttribute(), EGameplayEffectAttributeCaptureSource::Source, true)
{
	RelevantAttributesToCapture.Add(IntelligenceCapture);
	RelevantAttributesToCapture.Add(MagicResistCapture);
	RelevantAttributesToCapture.Add(ArmorCapture);
	RelevantAttributesToCapture.Add(CritChanceCapture);
	RelevantAttributesToCapture.Add(CritMultCapture);
}

void UDFDamageCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const UAbilitySystemComponent* const TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}

	FAggregatorEvaluateParameters EvalParams;
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float Intel = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(IntelligenceCapture, EvalParams, Intel);

	float MR = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MagicResistCapture, EvalParams, MR);

	float Armor = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(ArmorCapture, EvalParams, Armor);
	(void)Armor;

	float CritChance = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(CritChanceCapture, EvalParams, CritChance);

	float CritMult = 2.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(CritMultCapture, EvalParams, CritMult);

	const FGameplayTag DataDamageTag = FDFGameplayTags::ResolveDataDamageTag();
	const float SetByCallerBase = DataDamageTag.IsValid()
		? Spec.GetSetByCallerMagnitude(DataDamageTag, false, 0.f)
		: 0.f;

	float PreMitigation = (SetByCallerBase + Intel * 0.5f) * (1.f - FMath::Clamp(MR, 0.f, 100.f) / 100.f);
	PreMitigation = FMath::Max(0.f, PreMitigation);

	const bool bCrit = FMath::FRand() < FMath::Clamp(CritChance, 0.f, 1.f);
	if (bCrit)
	{
		PreMitigation *= FMath::Max(1.f, CritMult);
	}

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UDFAttributeSet::GetHealthAttribute(),
		EGameplayModOp::Additive, -PreMitigation));
}
