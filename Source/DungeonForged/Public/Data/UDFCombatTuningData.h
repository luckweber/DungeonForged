// Source/DungeonForged/Public/Data/UDFCombatTuningData.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/DFMeleeTraceTypes.h"
#include "GameplayTagContainer.h"
#include "UDFCombatTuningData.generated.h"

class UNiagaraSystem;
class USoundBase;

USTRUCT(BlueprintType)
struct FDFImpactFeedbackAssets
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Feedback")
	TObjectPtr<UNiagaraSystem> ImpactVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Feedback")
	TObjectPtr<USoundBase> ImpactSFX = nullptr;
};

/** Central combat / feel numbers for editor tuning without recompile. */
UCLASS(BlueprintType)
class DUNGEONFORGED_API UDFCombatTuningData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee")
	float ComboWindowDuration = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee")
	float HeavyChargeThreshold = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee")
	float HeavyDamageMultiplier = 2.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee")
	float HeavyKnockbackMultiplier = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee")
	float HeavyStaminaCost = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee")
	float HeavyTraceRadiusBonus = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee")
	float AttackInputBufferDuration = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge")
	float DodgeIFrameDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge")
	float DodgeCooldown = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|State")
	float CombatExitDelay = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss")
	float BossVulnerabilityDuration = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxCooldownReduction = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxCritChance = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stamina")
	float StaminaRegenInCombat = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stamina")
	float StaminaRegenOutOfCombat = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stamina")
	float ExhaustedStaminaThreshold = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stamina")
	float ExhaustedLockDuration = 0.5f;

	/** Minimum CC duration multiplier after StatusResist (0.15 = at least 15% of base duration). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float StatusResistMinDurationScale = 0.15f;

	/** Optional per-tag impact VFX/SFX (keys: Impact.Light.Slash, Impact.Heavy.Blunt, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Feedback", meta = (Categories = "Impact"))
	TMap<FGameplayTag, FDFImpactFeedbackAssets> ImpactFeedbackByTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities")
	bool bEnableGlobalAbilityGCD = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (ClampMin = "0.0"))
	float GlobalAbilityGCD = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee|Trace", meta = (ClampMin = "1", ClampMax = "8"))
	int32 DefaultMeleeTraceSubSteps = 3;

	/** Per weapon tag melee sweep shape (B2). Most specific matching tag wins. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee|Trace", meta = (Categories = "Weapon"))
	TMap<FGameplayTag, EDFMeleeTraceShape> TraceShapeByWeaponTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finisher", meta = (ClampMin = "0.05", ClampMax = "0.5"))
	float FinisherHealthThreshold = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finisher", meta = (ClampMin = "1", ClampMax = "6"))
	int32 FinisherMultiHitCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finisher", meta = (ClampMin = "0.1"))
	float FinisherInputWindowSec = 0.65f;
};
