// Source/DungeonForged/Public/GAS/UDFPassivesGASEvents.h
#pragma once

#include "CoreMinimal.h"

struct FGameplayEffectSpec;
class UAbilitySystemComponent;

/**
 * Global passive hooks from UDFAttributeSet (hit received, lethal kills) and Rogue bleed broadcast.
 * Events route through UAbilitySystemComponent::HandleGameplayEvent for passive ability tasks.
 */
namespace UDFPassivesGASEvents
{
/** Victim: Event.Hit.Received (EventMagnitude = damage taken this execute). */
DUNGEONFORGED_API void DispatchHitReceived(UAbilitySystemComponent* VictimASC, const FGameplayEffectSpec& Spec, float Damage);
/**
 * If victim health is zero after a damaging execute, and the context carries a gameplay ability,
 * send Event.Ability.Kill to the killer (for Mana Vortex, etc.).
 */
DUNGEONFORGED_API void DispatchLethalAbilityKillIfAny(UAbilitySystemComponent* VictimASC, const FGameplayEffectSpec& Spec);
/**
 * Call after UGE_DoT_Bleed is applied to a target; notifies instigator (Rogue Bleed Mastery).
 * Instigator: Event.Passive.Rogue.BleedApplied; Target in payload = Victim.
 */
DUNGEONFORGED_API void DispatchRogueBleedApplied(AActor* InstigatorAvatar, AActor* Victim, UAbilitySystemComponent* InstigatorASC);
} // namespace UDFPassivesGASEvents
