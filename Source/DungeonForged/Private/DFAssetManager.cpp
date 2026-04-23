// Source/DungeonForged/Private/DFAssetManager.cpp
// UDFAssetManager: tag registration + primary assets. GAS InitGlobalData is in UDungeonForgedGameInstance::Init.
#include "DFAssetManager.h"
#include "Engine/AssetManager.h"
#include "GAS/DFGameplayTags.h"

UDFAssetManager& UDFAssetManager::Get()
{
	// DefaultEngine: AssetManagerClassName=/Script/DungeonForged.DFAssetManager (nome refletido sem "U"; class C++: UDFAssetManager).
	// CastChecked fails if a instância não for UDFAssetManager (ini errada ou manager ainda o UAssetManager padrão).
	return *CastChecked<UDFAssetManager>(&UAssetManager::Get());
}

void UDFAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
	FDFGameplayTags::RegisterGameplayTags();
}
