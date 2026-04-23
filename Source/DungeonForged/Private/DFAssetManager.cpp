// Source/DungeonForged/Private/DFAssetManager.cpp
// UDFAssetManager: primary-asset / streaming only. GAS init is in UDungeonForgedGameInstance::Init.
#include "DFAssetManager.h"
#include "Engine/AssetManager.h"

UDFAssetManager& UDFAssetManager::Get()
{
	// DefaultEngine: AssetManagerClassName=/Script/DungeonForged.DFAssetManager (nome refletido sem "U"; class C++: UDFAssetManager).
	// CastChecked fails if a instância não for UDFAssetManager (ini errada ou manager ainda o UAssetManager padrão).
	return *CastChecked<UDFAssetManager>(&UAssetManager::Get());
}

void UDFAssetManager::StartInitialLoading()
{
	// GAS + tags: see UDungeonForgedGameInstance::Init (avoids InitGlobalData during early asset manager boot — that timing caused crashes).
	Super::StartInitialLoading();
}
