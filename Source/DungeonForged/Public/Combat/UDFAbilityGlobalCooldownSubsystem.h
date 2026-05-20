// Source/DungeonForged/Public/Combat/UDFAbilityGlobalCooldownSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UDFAbilityGlobalCooldownSubsystem.generated.h"

class UAbilitySystemComponent;

/** Optional global ability cooldown layer (B11 / G2). */
UCLASS()
class DUNGEONFORGED_API UDFAbilityGlobalCooldownSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	bool IsGlobalCooldownReady(const UAbilitySystemComponent* ASC, float GCDSeconds) const;
	void MarkGlobalCooldownUsed(UAbilitySystemComponent* ASC);

private:
	TMap<TWeakObjectPtr<UAbilitySystemComponent>, double> LastActivationRealTime;
};
