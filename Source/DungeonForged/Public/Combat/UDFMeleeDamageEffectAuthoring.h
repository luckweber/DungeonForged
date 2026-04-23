// Source/DungeonForged/Public/Combat/UDFMeleeDamageEffectAuthoring.h
#pragma once

#include "CoreMinimal.h"

/**
 * @file UDFMeleeDamageEffectAuthoring.h
 *
 * Asset authoring: GE_MeleeDamage (and optional GE_Finishing)
 *
 * GE_MeleeDamage (Gameplay Effect, Blueprint or native subclass of UGameplayEffect)
 * - Duration: Instant.
 * - Executions: add an Execution Definition with Calculation class = UDFDamageCalculation
 *   (Source/DungeonForged/Public/GAS/DFDamageCalculation.h). That class reads
 *   SetByCaller "Data.Damage" and applies additive damage to the target UDFAttributeSet::Health
 *   (and optional crit/mitigation in the same execution). Data.Knockback is passed on the
 *   spec for UDFHitReactionComponent / physics but is not read by UDFDamageCalculation
 *   unless you extend the execution to use it.
 * - SetByCaller Magnitudes: map tags Data.Damage and Data.Knockback in the effect asset
 *   (Magnitude: Set by Caller, Magnitude Scalable Float). UDFMeleeTraceComponent::BuildDamageSpec
 *   fills them at runtime. Native tags: FDFGameplayTags::Data_Damage, Data_Knockback
 *   ("Data.Damage", "Data.Knockback").
 * - Target tags / granted tags on the damage GE: default none. Stagger, hit reactions, and
 *   State.Stunned for short stagger windows are applied by UDFHitReactionComponent from
 *   its own TSubclassOf<UGameplayEffect> StaggerStunGameplayEffect.
 * - No gameplay cues required for core logic; you may add Cues for VFX in parallel.
 *
 * GE_Finishing (optional, assigned on UDFMeleeTraceComponent)
 * - Instant (or your designer choice) effect that applies a bonus to execution damage or
 *   a separate health modifier, triggered when the target is below
 *   FinishingHealthFractionThreshold of max health. Configure SetByCaller on that GE
 *   (e.g. Data.Damage) and set FinishingSetByCallerTag + FinishingSetByCallerMagnitude
 *   on the trace component, or use a dedicated GameplayEffectExecutionCalculation.
 *
 * Native tags: ensure FDFGameplayTags::RegisterGameplayTags() has run (game instance / asset
 * manager init) before relying on FDFGameplayTags::Data_* in code.
 */

namespace DF
{
	/** Intentionally empty — documentation for designers lives in the file comment block above. */
	static constexpr int32 MeleeEffectAuthoringDocOnly = 0;
}
