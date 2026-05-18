// Source/DungeonForged/Public/Data/UDFCombatTuningData.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UDFCombatTuningData.generated.h"

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
	float AttackInputBufferDuration = 0.15f;

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
};
