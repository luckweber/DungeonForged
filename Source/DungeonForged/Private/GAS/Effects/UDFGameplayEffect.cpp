// Source/DungeonForged/Private/GAS/Effects/UDFGameplayEffect.cpp
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UObject/Object.h"

void UDFGameplayEffect::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		ConfigureEffectCDO();
		OnGameplayEffectChanged();
	}
}

void UDFGameplayEffect::ConfigureEffectCDO() {}
