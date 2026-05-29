// Source/DungeonForged/Private/GAS/Universal/UDFUniversalAbilityRegistry.cpp
#include "GAS/Universal/UDFUniversalAbilityRegistry.h"
#include "GAS/Abilities/Universal/UDFAbility_Universal_BattleHymn.h"
#include "GAS/Abilities/Universal/UDFAbility_Universal_Berserk.h"
#include "GAS/Abilities/Universal/UDFAbility_Universal_CallLightning.h"
#include "GAS/Abilities/Universal/UDFAbility_Universal_HealthPotion.h"
#include "GAS/Abilities/Universal/UDFAbility_Universal_SecondWind.h"
#include "GAS/Abilities/Universal/UDFAbility_Universal_Siphon.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFGameplayAbility.h"

TSubclassOf<UDFGameplayAbility> UDFUniversalAbilityRegistry::ResolveAbilityClassFromTag(const FGameplayTag AbilityTag)
{
	if (!AbilityTag.IsValid())
	{
		return nullptr;
	}
	if (AbilityTag == FDFGameplayTags::Ability_Universal_HealthPotion)
	{
		return UDFAbility_Universal_HealthPotion::StaticClass();
	}
	if (AbilityTag == FDFGameplayTags::Ability_Universal_SecondWind)
	{
		return UDFAbility_Universal_SecondWind::StaticClass();
	}
	if (AbilityTag == FDFGameplayTags::Ability_Universal_BattleHymn)
	{
		return UDFAbility_Universal_BattleHymn::StaticClass();
	}
	if (AbilityTag == FDFGameplayTags::Ability_Universal_Siphon)
	{
		return UDFAbility_Universal_Siphon::StaticClass();
	}
	if (AbilityTag == FDFGameplayTags::Ability_Universal_Berserk)
	{
		return UDFAbility_Universal_Berserk::StaticClass();
	}
	if (AbilityTag == FDFGameplayTags::Ability_Universal_CallLightning)
	{
		return UDFAbility_Universal_CallLightning::StaticClass();
	}
	return nullptr;
}
