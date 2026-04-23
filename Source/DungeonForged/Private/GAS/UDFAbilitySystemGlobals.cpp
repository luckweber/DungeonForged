// Source/DungeonForged/Private/GAS/UDFAbilitySystemGlobals.cpp
#include "GAS/UDFAbilitySystemGlobals.h"
#include "GameplayEffectTypes.h"

FGameplayEffectContext* UDFAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FGameplayEffectContext();
}
