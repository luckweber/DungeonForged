// Source/DungeonForged/Public/GAS/Abilities/UDFAbility_Enemy_Melee.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "GameplayTagContainer.h"
#include "UDFAbility_Enemy_Melee.generated.h"

class UNiagaraSystem;
class USoundBase;
class ACharacter;

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

	/**
	 * If set, the hit frame is driven by an AnimNotify that sends this GameplayEvent.
	 * If unset, the legacy HitWindowDelay timer drives the hit frame.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Enemy|Timing", meta = (Categories = "Event"))
	FGameplayTag HitGameplayEventTag;

	/** Plays when the hit window opens, even if no target is hit. Good for slime bite/splash from mouth. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Enemy|FX")
	TObjectPtr<USoundBase> AttackSound = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Enemy|FX")
	TObjectPtr<UNiagaraSystem> AttackVFX = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Enemy|FX")
	FName AttackFXSocketName = FName("Mouth");

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Enemy|FX")
	FVector AttackVFXScale = FVector(1.f);

	/** Plays at each damaged target location. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Enemy|FX")
	TObjectPtr<USoundBase> ImpactSound = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Enemy|FX")
	TObjectPtr<UNiagaraSystem> ImpactVFX = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Enemy|FX")
	FVector ImpactVFXScale = FVector(1.f);

	virtual void PostInitProperties() override;
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UFUNCTION()
	void OnHitWindowElapsed();

	UFUNCTION()
	void OnHitGameplayEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageOrInstantFinished();

	void ExecuteHitWindow();
	void ApplyDamageToOverlappingTargets();
	void PlayAttackCosmetics(ACharacter* SourceCharacter) const;
	void PlayImpactCosmetics(ACharacter* SourceCharacter, const FVector& HitLocation, const FVector& HitNormal) const;
	void TryEndWhenIdle();

	int32 ActiveParallelTasks = 0;
	bool bHitWindowDone = false;
	bool bMontageFinishHandled = false;
};
