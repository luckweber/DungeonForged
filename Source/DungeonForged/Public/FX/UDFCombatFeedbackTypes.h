// Source/DungeonForged/Public/FX/UDFCombatFeedbackTypes.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UDFCombatFeedbackTypes.generated.h"

UENUM(BlueprintType)
enum class EDFHitFeedbackBand : uint8
{
	Light		UMETA(DisplayName = "Light"),
	Heavy		UMETA(DisplayName = "Heavy"),
	Critical	UMETA(DisplayName = "Critical (high % of max health)"),
	Knockback	UMETA(DisplayName = "Knockback"),
};

/** Unified payload for hit confirmation (gameplay + juice). */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFHitConfirmedContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TObjectPtr<AActor> Instigator = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TObjectPtr<AActor> Victim = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FVector Normal = FVector::UpVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FVector HitDirection2D = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float Magnitude = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float KnockbackMagnitude = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MaxHealth = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DamagePercent = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bIsCrit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FGameplayTag DamageSourceTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FGameplayTagContainer Tags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	EDFHitFeedbackBand Band = EDFHitFeedbackBand::Light;

	/** Resolved Impact.* tag (band × damage source) for VFX/SFX lookup. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FGameplayTag ImpactTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FName HitBoneName = NAME_None;

	/** True when this hit reduced the victim to zero HP (lethal blow band boost). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bWasLethal = false;
};
