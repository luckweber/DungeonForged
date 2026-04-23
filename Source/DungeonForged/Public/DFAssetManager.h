// Source/DungeonForged/Public/DFAssetManager.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "DFAssetManager.generated.h"

/**
 * Optional custom asset manager (primary assets, bundles). Enable via AssetManagerClassName in DefaultEngine.ini when needed.
 * GAS is initialized in UDungeonForgedGameInstance::Init, not here.
 */
UCLASS()
class DUNGEONFORGED_API UDFAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	static UDFAssetManager& Get();

protected:
	virtual void StartInitialLoading() override;
};
