// Source/DungeonForged/Public/FX/UDFCombatFeedbackLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "FX/UDFCombatFeedbackTypes.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UDFCombatFeedbackLibrary.generated.h"

class AActor;

/** Central dispatch for hit-stop and camera shake bands. */
UCLASS()
class DUNGEONFORGED_API UDFCombatFeedbackLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DF|Combat|Feedback", meta = (WorldContext = "WorldContextObject"))
	static void PlayHitFeedbackForBand(
		UObject* WorldContextObject,
		EDFHitFeedbackBand Band,
		AActor* InstigatorActor = nullptr);

	UFUNCTION(BlueprintCallable, Category = "DF|Combat|Feedback", meta = (WorldContext = "WorldContextObject"))
	static void PlayHitFeedbackFromDamage(
		UObject* WorldContextObject,
		float DamageMagnitude,
		float MaxHealth,
		bool bIsKnockback,
		AActor* InstigatorActor = nullptr);

	/** Resolves feedback band from damage vs max health (when @a Band is still Light). */
	UFUNCTION(BlueprintPure, Category = "DF|Combat|Feedback")
	static EDFHitFeedbackBand ResolveFeedbackBand(
		float DamageMagnitude,
		float MaxHealth,
		bool bIsCrit,
		bool bIsKnockback,
		float HeavyDamageThreshold = 25.f);

	/**
	 * Single entry for hit confirmation: server hit reaction + combo refresh, then local hit-stop / shake / screen FX.
	 * Skips dedicated server for client-only juice.
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|Combat|Feedback", meta = (WorldContext = "WorldContextObject"))
	static void DispatchOnHitConfirmed(UObject* WorldContextObject, const FDFHitConfirmedContext& Context);

	/** Convenience wrapper after a projectile / AoE applies damage on authority. */
	UFUNCTION(BlueprintCallable, Category = "DF|Combat|Feedback", meta = (WorldContext = "WorldContextObject"))
	static void DispatchProjectileHitConfirmed(
		UObject* WorldContextObject,
		AActor* Instigator,
		AActor* Victim,
		const FHitResult& Hit,
		float DamageMagnitude,
		float KnockbackMagnitude = 0.f,
		FGameplayTag DamageSourceTag = FGameplayTag(),
		bool bIsCrit = false);

	/** Resolves Impact.{Band}.{Slash|Blunt|Pierce} from feedback band + damage source tag. */
	UFUNCTION(BlueprintPure, Category = "DF|Combat|Feedback")
	static FGameplayTag ResolveImpactTag(EDFHitFeedbackBand Band, FGameplayTag DamageSourceTag);

	/** Attacker-side hit-stop + camera shake (local or via Client_OnAttackHitConfirmed). */
	UFUNCTION(BlueprintCallable, Category = "DF|Combat|Feedback", meta = (WorldContext = "WorldContextObject"))
	static void DispatchAttackerHitFeel(UObject* WorldContextObject, const FDFHitConfirmedContext& Context);

	/** Victim-side screen FX when the victim is locally controlled. */
	UFUNCTION(BlueprintCallable, Category = "DF|Combat|Feedback", meta = (WorldContext = "WorldContextObject"))
	static void DispatchVictimHitFeel(UObject* WorldContextObject, const FDFHitConfirmedContext& Context);

	/** Tag outgoing damage specs so combat text is not duplicated in AttributeSet (A1). */
	static void MarkSpecCombatFeedbackCentralized(struct FGameplayEffectSpec& Spec);
};
