// Source/DungeonForged/Private/DFAssetManager.cpp
// UDFAssetManager: tag registration + primary assets. GAS InitGlobalData is in UDFGameInstance::Init.
#include "DFAssetManager.h"
#include "Data/UDFCombatTuningData.h"
#include "Engine/AssetManager.h"
#include "GAS/DFGameplayTags.h"

UDFAssetManager& UDFAssetManager::Get()
{
	// DefaultEngine: AssetManagerClassName=/Script/DungeonForged.DFAssetManager (nome refletido sem "U"; class C++: UDFAssetManager).
	// CastChecked fails if a instância não for UDFAssetManager (ini errada ou manager ainda o UAssetManager padrão).
	return *CastChecked<UDFAssetManager>(&UAssetManager::Get());
}

const UDFCombatTuningData* UDFAssetManager::GetCombatTuningDataSafe()
{
	if (!UAssetManager::IsInitialized())
	{
		return GetDefault<UDFCombatTuningData>();
	}
	if (const UDFAssetManager* const DFAM = Cast<UDFAssetManager>(&UAssetManager::Get()))
	{
		return DFAM->GetCombatTuningData();
	}
	return GetDefault<UDFCombatTuningData>();
}

void UDFAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
	FDFGameplayTags::RegisterGameplayTags();
}

const UDFCombatTuningData* UDFAssetManager::GetCombatTuningData() const
{
	if (CombatTuningDataAsset.IsNull())
	{
		return GetDefault<UDFCombatTuningData>();
	}
	if (UDFCombatTuningData* const Loaded = CombatTuningDataAsset.Get())
	{
		return Loaded;
	}
	if (UDFCombatTuningData* const Sync = CombatTuningDataAsset.LoadSynchronous())
	{
		return Sync;
	}
	return GetDefault<UDFCombatTuningData>();
}
