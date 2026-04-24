// Source/DungeonForged/Public/GAS/Abilities/Boss/DFBossAbilityCommons.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class AActor;
class UAbilitySystemComponent;
class UGameplayEffect;
class UWorld;

namespace DFBossAbilityCommons
{
DUNGEONFORGED_API ACharacter* GetFirstPlayerCharacter(UWorld* World, AActor* Ignore);

DUNGEONFORGED_API void ApplySetCallerCooldown(
	UAbilitySystemComponent* ASC, TSubclassOf<UGameplayEffect> CooldownClass, float Seconds);

DUNGEONFORGED_API void StunTarget(UAbilitySystemComponent* SourceOwnerASC, AActor* Target, float Duration);
}
