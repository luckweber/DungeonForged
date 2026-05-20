// Source/DungeonForged/Public/Combat/UDFDeathCinematicTypes.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UDFDeathCinematicTypes.generated.h"

/** Payload for player death / enemy kill cinematic dispatch (Silent Death report §5.3). */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFDeathCinematicContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	TObjectPtr<AActor> Victim = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	TObjectPtr<AActor> Killer = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	FName KillerDisplayName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	FVector LethalImpactLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	FVector LethalImpactDirection = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	FGameplayTagContainer DamageTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	bool bWasCrit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	bool bIsLastEnemyOfRoom = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	bool bIsBoss = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	float FinalDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	float ExperienceReward = 0.f;
};
