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
	, StrengthCapture(UDFAttributeSet::GetStrengthAttribute(), EGameplayEffectAttributeCaptureSource::Source, true)
	, MagicResistCapture(UDFAttributeSet::GetMagicResistAttribute(), EGameplayEffectAttributeCaptureSource::Target, true)
	, ArmorCapture(UDFAttributeSet::GetArmorAttribute(), EGameplayEffectAttributeCaptureSource::Target, true)
	, CritChanceCapture(UDFAttributeSet::GetCritChanceAttribute(), EGameplayEffectAttributeCaptureSource::Source, true)
	, CritMultCapture(UDFAttributeSet::GetCritMultiplierAttribute(), EGameplayEffectAttributeCaptureSource::Source, true)
	, SpellDamageAmpCapture(UDFAttributeSet::GetSpellDamageAmpAttribute(), EGameplayEffectAttributeCaptureSource::Source, true)
	, DodgeChanceCapture(UDFAttributeSet::GetDodgeChanceAttribute(), EGameplayEffectAttributeCaptureSource::Target, true)
	, BlockChanceCapture(UDFAttributeSet::GetBlockChanceAttribute(), EGameplayEffectAttributeCaptureSource::Target, true)
{
	RelevantAttributesToCapture.Add(IntelligenceCapture);
	RelevantAttributesToCapture.Add(StrengthCapture);
	RelevantAttributesToCapture.Add(MagicResistCapture);
	RelevantAttributesToCapture.Add(ArmorCapture);
	RelevantAttributesToCapture.Add(CritChanceCapture);
	RelevantAttributesToCapture.Add(CritMultCapture);
	RelevantAttributesToCapture.Add(SpellDamageAmpCapture);
	RelevantAttributesToCapture.Add(DodgeChanceCapture);
	RelevantAttributesToCapture.Add(BlockChanceCapture);
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

	float Str = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(StrengthCapture, EvalParams, Str);

	float MR = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MagicResistCapture, EvalParams, MR);

	float Armor = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(ArmorCapture, EvalParams, Armor);

	float CritChance = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(CritChanceCapture, EvalParams, CritChance);

	float CritMult = 2.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(CritMultCapture, EvalParams, CritMult);

	float SpellAmp = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(SpellDamageAmpCapture, EvalParams, SpellAmp);

	float Dodge = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DodgeChanceCapture, EvalParams, Dodge);

	float Block = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(BlockChanceCapture, EvalParams, Block);

	const float DodgeEff = FMath::Clamp(Dodge, 0.f, 0.75f);
	if (DodgeEff > KINDA_SMALL_NUMBER && FMath::FRand() < DodgeEff)
	{
		return;
	}

	const FGameplayTag DataDamageTag = FDFGameplayTags::ResolveDataDamageTag();
	const float SetByCallerBase = DataDamageTag.IsValid()
		? Spec.GetSetByCallerMagnitude(DataDamageTag, false, 0.f)
		: 0.f;

	static const FGameplayTagContainer GEmpty;
	const FGameplayTagContainer& AssetTags = Spec.Def ? Spec.Def->GetAssetTags() : GEmpty;
	const bool bPhysical = Spec.Def && AssetTags.HasTag(FDFGameplayTags::Effect_Damage_Physical);
	const bool bMagic = Spec.Def && AssetTags.HasTag(FDFGameplayTags::Effect_Damage_Magic);

	float PreMitigation = 0.f;
	if (bPhysical)
	{
		const float ArmorK = 100.f;
		const float ArmorReduction = Armor / (Armor + ArmorK);
		PreMitigation = (SetByCallerBase + Str * 0.5f) * (1.f - FMath::Clamp(ArmorReduction, 0.f, 0.85f));
	}
	else if (bMagic)
	{
		PreMitigation = (SetByCallerBase + Intel * 0.5f) * (1.f + FMath::Max(0.f, SpellAmp)) * (1.f - FMath::Clamp(MR, 0.f, 100.f) / 100.f);
	}
	else
	{
		// Legacy: treat as magic if asset tags are missing
		PreMitigation = (SetByCallerBase + Intel * 0.5f) * (1.f - FMath::Clamp(MR, 0.f, 100.f) / 100.f);
	}
	PreMitigation = FMath::Max(0.f, PreMitigation);

	if (FDFGameplayTags::State_Combat_Block.IsValid()
		&& TargetASC->HasMatchingGameplayTag(FDFGameplayTags::State_Combat_Block))
	{
		PreMitigation *= (1.f - FMath::Clamp(Block, 0.f, 0.75f));
	}

	if (FDFGameplayTags::State_BossVulnerable.IsValid()
		&& TargetASC->HasMatchingGameplayTag(FDFGameplayTags::State_BossVulnerable))
	{
		PreMitigation *= 1.5f;
	}

	const float CritRaw = FMath::Clamp(CritChance, 0.f, 1.f);
	const float CritEffective = CritRaw <= 0.5f ? CritRaw : 0.5f + (CritRaw - 0.5f) * 0.6f;
	const bool bCrit = FMath::FRand() < FMath::Clamp(CritEffective, 0.f, 0.75f);
	if (bCrit)
	{
		PreMitigation *= FMath::Max(1.f, CritMult);
	}
	FGameplayEffectSpec& MutSpec = const_cast<FGameplayEffectSpec&>(Spec);
	if (FDFGameplayTags::Data_CriticalHit.IsValid())
	{
		MutSpec.SetSetByCallerMagnitude(FDFGameplayTags::Data_CriticalHit, bCrit ? 1.f : 0.f);
	}

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UDFAttributeSet::GetHealthAttribute(),
		EGameplayModOp::Additive, -PreMitigation));
}
