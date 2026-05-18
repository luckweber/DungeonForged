// Source/DungeonForged/Public/DFAssetManager.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "DFAssetManager.generated.h"

class UDFCombatTuningData;

/**
 * Project asset manager: FDFGameplayTags::RegisterGameplayTags in StartInitialLoading, then primary-asset work.
 * UAbilitySystemGlobals::InitGlobalData remains in UDFGameInstance::Init.
 */
UCLASS()
class DUNGEONFORGED_API UDFAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	static UDFAssetManager& Get();

	/** Loaded once from @c CombatTuningDataAsset; falls back to class CDO when unset. */
	UFUNCTION(BlueprintPure, Category = "DF|Data")
	const UDFCombatTuningData* GetCombatTuningData() const;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Data")
	TSoftObjectPtr<UDFCombatTuningData> CombatTuningDataAsset;

protected:
	virtual void StartInitialLoading() override;
};
