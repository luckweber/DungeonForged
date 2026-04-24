// Source/DungeonForged/Public/DFAssetManager.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "DFAssetManager.generated.h"

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

protected:
	virtual void StartInitialLoading() override;
};
