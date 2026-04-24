// Source/DungeonForged/Public/Run/DFSaveGame.h
#pragma once

#include "CoreMinimal.h"
#include "Localization/DFAccessibilityData.h"
#include "Localization/DFLocalizationTypes.h"
#include "GameFramework/SaveGame.h"
#include "DFSaveGame.generated.h"

/**
 * Persistent meta-progression (high score, unlocks, run stats). Uses UPROPERTY(SaveGame) + UGameplayStatics slot API.
 * @see ue-serialization-savegames
 */
UCLASS(BlueprintType)
class DUNGEONFORGED_API UDFSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Bumped when fields change; used for future migrations. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta")
	int32 SaveVersion = 2;

	/** @see UDFLocalizationSubsystem */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Settings|Localization")
	EDFLanguage PreferredLanguage = EDFLanguage::PortugueseBrazil;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Settings|Localization")
	FString PreferredCultureCode = TEXT("pt-BR");

	/** @see UDFAccessibilitySubsystem */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Settings|Accessibility")
	FDFAccessibilitySettings AccessibilitySettings;

	/** Enhanced Input mapping name → FKey::ToString() for runtime remap reapply. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Settings|Input")
	TMap<FName, FString> SavedKeyBindings;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta")
	int32 HighScore = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta")
	int32 TotalRuns = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta")
	int32 TotalWins = 0;

	/** Class row names in DT_Classes the player can select. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta|Unlocks")
	TArray<FName> UnlockedClasses;

	/** Row names in DT_Abilities that are permanently unlocked (meta). */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta|Unlocks")
	TArray<FName> UnlockedAbilities;

	/** Synchronous load from the primary meta slot, or a new object if missing / invalid. */
	UFUNCTION(BlueprintCallable, Category = "DF|Meta|Save")
	static UDFSaveGame* Load();

	/** Synchronous write to the primary meta slot. */
	UFUNCTION(BlueprintCallable, Category = "DF|Meta|Save")
	static bool Save(UDFSaveGame* Data);

	static const TCHAR* GetSlotName() { return TEXT("DFMetaSave"); }
	static constexpr int32 UserIndex = 0;
};
