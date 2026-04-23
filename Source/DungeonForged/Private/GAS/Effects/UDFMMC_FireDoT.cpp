// Source/DungeonForged/Private/GAS/Effects/UDFMMC_FireDoT.cpp
#include "GAS/Effects/UDFMMC_FireDoT.h"
#include "GAS/UDFAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"

UDFMMC_FireDoT::UDFMMC_FireDoT()
	: UGameplayModMagnitudeCalculation()
	, StrengthCapture(UDFAttributeSet::GetStrengthAttribute(), EGameplayEffectAttributeCaptureSource::Target, true)
{
	RelevantAttributesToCapture.Add(StrengthCapture);
}

float UDFMMC_FireDoT::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	FAggregatorEvaluateParameters Eval;
	Eval.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	Eval.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	float S = 0.f;
	if (!GetCapturedAttributeMagnitude(StrengthCapture, Spec, Eval, S))
	{
		return 0.f;
	}
	return -FMath::Max(0.f, S) * 0.1f;
}
