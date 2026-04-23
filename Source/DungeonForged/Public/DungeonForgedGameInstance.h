// Source/DungeonForged/Public/DungeonForgedGameInstance.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "DungeonForgedGameInstance.generated.h"

/**
 * GAS + native gameplay tags init lives here (safe timing). UDFAssetManager::StartInitialLoading stays Super-only
 * so you can use a custom asset manager (primary assets) without running InitGlobalData during engine boot.
 */
UCLASS()
class DUNGEONFORGED_API UDungeonForgedGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
};
