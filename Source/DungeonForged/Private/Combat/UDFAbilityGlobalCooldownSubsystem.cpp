// Source/DungeonForged/Private/Combat/UDFAbilityGlobalCooldownSubsystem.cpp
#include "Combat/UDFAbilityGlobalCooldownSubsystem.h"

#include "AbilitySystemComponent.h"
#include "HAL/PlatformTime.h"

bool UDFAbilityGlobalCooldownSubsystem::IsGlobalCooldownReady(
	const UAbilitySystemComponent* const ASC,
	const float GCDSeconds) const
{
	if (!ASC || GCDSeconds <= KINDA_SMALL_NUMBER)
	{
		return true;
	}
	const TWeakObjectPtr<UAbilitySystemComponent> Key(const_cast<UAbilitySystemComponent*>(ASC));
	if (const double* const Last = LastActivationRealTime.Find(Key))
	{
		return (FPlatformTime::Seconds() - *Last) >= static_cast<double>(GCDSeconds) - KINDA_SMALL_NUMBER;
	}
	return true;
}

void UDFAbilityGlobalCooldownSubsystem::MarkGlobalCooldownUsed(UAbilitySystemComponent* const ASC)
{
	if (!ASC)
	{
		return;
	}
	LastActivationRealTime.Add(TWeakObjectPtr<UAbilitySystemComponent>(ASC), FPlatformTime::Seconds());
}
