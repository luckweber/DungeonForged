// Source/DungeonForged/Public/GAS/DFRogueGAS.h
#pragma once

#include "Engine/EngineTypes.h"
#include "AbilitySystemComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffectTypes.h"

/** Physical damage: project uses (SetByCaller + 0.5*Str) * armor; we pass a compensated SetByCaller for agility-first formulas. */
inline float DF_Rogue_CompensatePhysicalSetBy(const UAbilitySystemComponent* ASC, const float IntendedPreArmorDamage)
{
	if (!ASC)
	{
		return 0.f;
	}
	const float Str = ASC->GetNumericAttribute(UDFAttributeSet::GetStrengthAttribute());
	return FMath::Max(0.f, IntendedPreArmorDamage - Str * 0.5f);
}

inline FGameplayEffectContextHandle DF_Rogue_EffectContext(UAbilitySystemComponent* ASC, AActor* Inst, const FHitResult* OptHit = nullptr)
{
	FGameplayEffectContextHandle C = ASC->MakeEffectContext();
	if (Inst)
	{
		C.AddInstigator(Inst, Inst);
	}
	if (OptHit)
	{
		C.AddHitResult(*OptHit);
	}
	return C;
}
