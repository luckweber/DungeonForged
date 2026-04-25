// Source/DungeonForged/Public/GAS/Abilities/UDFAbility_Enemy_Melee.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Enemy_Melee.generated.h"

/**
 * Melee default for any `ACharacter` with an ASC (e.g. `ADFEnemyBase` / Slime).
 * Tagged with `Ability.Attack.Melee` for `UDFBTTask_MeleeAttack` + `GrantedAbilitiesByTag` on the enemy.
 */
UCLASS()
class DUNGEONFORGED_API UDFAbility_Enemy_Melee : public UDFGameplayAbility
{
	GENERATED_BODY()
public:
	UDFAbility_Enemy_Melee();

	/** After activation, time until `UGE_Damage_Physical` is applied (align with a swing notify). */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Enemy", meta = (ClampMin = "0.0"))
	float HitWindowDelay = 0.2f;

	/** `Data.Damage` = `Strength` * this (enemies set Strength from `FDFEnemyTableRow::BaseDamage`). */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Enemy", meta = (ClampMin = "0.0"))
	float DamageFromStrengthScale = 1.f;

	/** Optional flat damage (added after Strength * scale). */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Enemy", meta = (ClampMin = "0.0"))
	float AddedDamage = 0.f;

	/** Sphere trace origin = avatar location + forward * this (cm). */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Enemy", meta = (ClampMin = "0.0"))
	float MeleeForwardOffset = 50.f;

	/** Overlap radius for the hit check (cm). */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Enemy", meta = (ClampMin = "1.0"))
	float MeleeRadius = 100.f;

	virtual void PostInitProperties() override;
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UFUNCTION()
	void OnHitWindowElapsed();

	UFUNCTION()
	void OnMontageOrInstantFinished();

	void ApplyDamageToOverlappingTargets();
	void TryEndWhenIdle();

	int32 ActiveParallelTasks = 0;
	bool bHitWindowDone = false;
	bool bMontageFinishHandled = false;
};
