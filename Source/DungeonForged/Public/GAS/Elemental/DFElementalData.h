// Source/DungeonForged/Public/GAS/Elemental/DFElementalData.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "DFElementalData.generated.h"

/** Fire ←→ Ice, Water, Earth, etc.; includes Water for the advantage matrix. */
UENUM(BlueprintType)
enum class EDFElementType : uint8
{
	None = 0 UMETA(Hidden),
	Fire		UMETA(DisplayName = "Fire"),
	Ice			UMETA(DisplayName = "Ice"),
	Water		UMETA(DisplayName = "Water"),
	Lightning	UMETA(DisplayName = "Lightning"),
	Earth		UMETA(DisplayName = "Earth"),
	Arcane		UMETA(DisplayName = "Arcane"),
	Physical	UMETA(DisplayName = "Physical"),
	/** Unmitigatable element (displays as "True" in editor). UHT forbids the identifier `True`. */
	ElementTrue	UMETA(DisplayName = "True"),
	/** C++ bounds / array sizing — not a valid row type. */
	MAX			UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FDFElementalResistanceEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Elemental")
	EDFElementType Element = EDFElementType::None;

	/** 1.0 = neutral; 0.5 = resist; 2.0 = weak to this element. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Elemental", meta = (ClampMin = "0.0"))
	float Multiplier = 1.f;
};

USTRUCT(BlueprintType)
struct FDFElementalReactionTagEntry
{
	GENERATED_BODY()

	/** When this element hits the target, use this tag for BP / GE resolution (e.g. frozen oil). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Elemental")
	EDFElementType WhenHitBy = EDFElementType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Elemental", meta = (Categories = "GameplayTag"))
	FGameplayTag ReactionTag;
};

/**
 * Per-enemy row in DT_EnemyElemental. Use TArray entries so CSV/DataTable import stays reliable.
 * @see UDFElementalComponent::InitFromTable
 */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFElementalAffinityRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Elemental")
	EDFElementType PrimaryElement = EDFElementType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Elemental")
	TArray<FDFElementalResistanceEntry> Resistances;

	/** Data-driven: incoming element ➜ reaction tag (e.g. oil + fire = custom reaction). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Elemental")
	TArray<FDFElementalReactionTagEntry> ReactionByIncomingElement;

	float GetResistance(const EDFElementType VsElement) const;
	FGameplayTag GetReactionForIncoming(const EDFElementType Incoming) const;
};
