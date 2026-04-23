// Source/DungeonForged/Private/GAS/Effects/UGE_Cost_Mana_Base.cpp
#include "GAS/Effects/UGE_Cost_Mana_Base.h"
#include "GAS/Effects/UDFMMC_NegateDataCost.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"

UGE_Cost_Mana_Base::UGE_Cost_Mana_Base()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Mod;
	Mod.Attribute = UDFAttributeSet::GetManaAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	FCustomCalculationBasedFloat Cc;
	Cc.CalculationClassMagnitude = UDFMMC_NegateDataCost::StaticClass();
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(Cc);
	Modifiers.Add(Mod);
}
