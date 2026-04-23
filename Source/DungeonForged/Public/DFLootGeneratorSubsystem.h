// Source/DungeonForged/Public/DFLootGeneratorSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Data/DFDataTableStructs.h"
#include "Subsystems/WorldSubsystem.h"
#include "DFLootGeneratorSubsystem.generated.h"

class ADFLootDrop;
class UDataTable;
class UWorld;

UCLASS()
class DUNGEONFORGED_API UDFLootGeneratorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot")
	TObjectPtr<UDataTable> ItemDataTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot")
	TSubclassOf<ADFLootDrop> LootDropClass;

	/**
	 * For each FName in EnemyData.LootTableRows, rolls a drop chance that scales with item rarity
	 * (common drops more often). Spawns a loot actor at SpawnLocation (offset per successful roll).
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|Loot")
	void RollLoot(const FDFEnemyTableRow& EnemyData, FVector SpawnLocation, FVector Impulse = FVector::ZeroVector);

	/**
	 * Spawns 2–4 guaranteed drops: builds a pool from LootPoolDataTable+LootTableRow, filters by MinRarity
	 * against ItemDataTable, then uses weighted row picks. Authority only.
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|Loot")
	void RollGuaranteedDropsFromPool(
		UDataTable* LootPoolDataTable,
		FName LootTableRow,
		UDataTable* InItemsForRarity,
		EItemRarity MinRarity,
		int32 MinCount,
		int32 MaxCount,
		FVector SpawnLocation,
		FVector BaseImpulse = FVector::ZeroVector);

	/** Single weighted pick from the list (rarity as weight) — utility for other systems. */
	UFUNCTION(BlueprintCallable, Category = "DF|Loot", meta = (AutoCreateRefTerm = "InRowNames"))
	static FName PickOneWeightedRow(const UDataTable* InItemDataTable, const TArray<FName>& InRowNames);

protected:
	/** Rarity as independent drop odds (sum used for per-row success roll). Tweak in defaults or BP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot|Rarity", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BaseDropChance = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot|Rarity", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CommonMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot|Rarity", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float UncommonMultiplier = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot|Rarity", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RareMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot|Rarity", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EpicMultiplier = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot|Rarity", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LegendaryMultiplier = 0.15f;

	/** Additional scatter so multiple drops from one kill do not z-fight. */
	UPROPERTY(EditAnywhere, Category = "DF|Loot", meta = (ClampMin = "0.0"))
	float LootScatterRadius = 40.f;

	float GetRarityMult(EItemRarity R) const;
};
