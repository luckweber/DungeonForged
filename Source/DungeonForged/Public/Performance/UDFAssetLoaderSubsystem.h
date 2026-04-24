// Source/DungeonForged/Public/Performance/UDFAssetLoaderSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Data/DFDataTableStructs.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/SoftObjectPath.h"
#include "Engine/StreamableManager.h"

#include "UDFAssetLoaderSubsystem.generated.h"

class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDFAbilityAssetsReady);

UCLASS()
class DUNGEONFORGED_API UDFAssetLoaderSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** If unset, UDFDungeonManager (game instance) is queried in `PreloadFloorAssets`. */
	UPROPERTY(EditAnywhere, Category = "DF|Preload")
	TObjectPtr<UDataTable> DungeonFloorTable = nullptr;

	UPROPERTY(EditAnywhere, Category = "DF|Preload")
	TObjectPtr<UDataTable> EnemyDataTable = nullptr;

	UPROPERTY(EditAnywhere, Category = "DF|Preload")
	TObjectPtr<UDataTable> AbilityDataTable = nullptr;

	UPROPERTY(BlueprintAssignable, Category = "DF|Preload|Events")
	FOnDFAbilityAssetsReady OnAbilityAssetsReady;

	/** Gathers classes/montages/FX from the dungeon floor row and async-loads their packages via `FStreamableManager` (and primary assets when registered). */
	UFUNCTION(BlueprintCallable, Category = "DF|Preload")
	void PreloadFloorAssets(int32 FloorNumber);

	/** Resolves `FDFAbilityTableRow` by name and requests async load of class + soft paths. Fires `OnAbilityAssetsReady` on completion. */
	UFUNCTION(BlueprintCallable, Category = "DF|Preload")
	void PreloadAbilityAssets(const TArray<FName>& AbilityRowNames);

protected:
	void AddEnemyRowPaths(const FDFEnemyTableRow& Row, TArray<FSoftObjectPath>& OutPaths) const;
	void AddAbilityRowPaths(const FDFAbilityTableRow& Row, TArray<FSoftObjectPath>& OutPaths) const;
	void StartAsyncLoad(const TArray<FSoftObjectPath>& Paths, TFunction<void()>&& OnDone, TSharedPtr<FStreamableHandle>& OutHandleSlot);

	/** In-flight streamable handles (restarted when a new request of the same kind is made). */
	TSharedPtr<FStreamableHandle> ActiveFloorLoad;
	TSharedPtr<FStreamableHandle> ActiveAbilityLoad;
};
