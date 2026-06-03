// Source/DungeonForged/Public/Run/UDFSaveLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UDFSaveLibrary.generated.h"

class UDFSaveGame;
class UDFSaveSlotManagerSubsystem;

/**
 * Canonical save IO — all gameplay code should use this instead of @c UDFSaveGame::Load().
 */
UCLASS()
class DUNGEONFORGED_API UDFSaveLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "DF|Save", meta = (WorldContext = "WorldContextObject"))
	static UDFSaveSlotManagerSubsystem* GetSaveSlots(const UObject* WorldContextObject);

	/** Ensures an active profile slot and returns mutable meta save (creates slot 0 if needed). */
	UFUNCTION(BlueprintCallable, Category = "DF|Save", meta = (WorldContext = "WorldContextObject"))
	static UDFSaveGame* ResolveMutableMetaSave(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "DF|Save", meta = (WorldContext = "WorldContextObject"))
	static UDFSaveGame* GetMutableMetaSave(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "DF|Save", meta = (WorldContext = "WorldContextObject"))
	static const UDFSaveGame* GetMetaSave(const UObject* WorldContextObject);

	/** Persists through @c UDFSaveSlotManagerSubsystem when a profile is active. */
	UFUNCTION(BlueprintCallable, Category = "DF|Save", meta = (WorldContext = "WorldContextObject"))
	static bool SaveMetaSave(const UObject* WorldContextObject, UDFSaveGame* SaveData);
};
