// Source/DungeonForged/Public/Combat/DFMeleeTraceTypes.h
#pragma once

#include "CoreMinimal.h"
#include "DFMeleeTraceTypes.generated.h"

UENUM(BlueprintType)
enum class EDFMeleeTraceShape : uint8
{
	Sphere	UMETA(DisplayName = "Sphere Sweep"),
	Capsule UMETA(DisplayName = "Capsule Sweep"),
	Cone	UMETA(DisplayName = "Cone (multi-sphere)"),
};

/** Extra overlapping trace zone for wide swings (shoulder / tip). */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFMeleeTraceZone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	FName StartSocket = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	FName EndSocket = NAME_None;

	/** Multiplies @c TraceRadius for this zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace", meta = (ClampMin = "0.1"))
	float RadiusScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	float RadiusOffset = 0.f;
};
