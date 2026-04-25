// Source/DungeonForged/Public/Run/DFSaveGame.h
#pragma once

#include "CoreMinimal.h"
#include "Localization/DFAccessibilityData.h"
#include "Localization/DFLocalizationTypes.h"
#include "GameFramework/SaveGame.h"
#include "GameModes/Nexus/DFNexusTypes.h"
#include "Run/DFRunManager.h"
#include "World/DFWorldTypes.h"
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
	int32 SaveVersion = 5;

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

	/** Nexus meta progression (mirrors @ref ADFNexusGameState on load). */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta|Nexus")
	int32 MetaXP = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta|Nexus")
	int32 MetaLevel = 1;

	/** NPC @c NPCId values that are visible in the Nexus. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta|Nexus")
	TArray<FName> UnlockedNPCs;

	/** Completed meta upgrade row names (Blacksmith, Merchant, etc.). */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta|Nexus")
	TArray<FName> CompletedUpgrades;

	/** Toast / queue: applied on next Nexus visit. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta|Nexus")
	TArray<FDFPendingUnlockEntry> PendingUnlocks;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta|Nexus")
	TArray<FDFRunHistoryEntry> RunHistory;

	/** Lifetime meta stats (Chronicler). */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta|Nexus")
	int32 LifetimeKills = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta|Nexus")
	int32 LifetimeDeaths = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta|Nexus")
	float TotalPlayTimeSeconds = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta|Nexus")
	int32 TotalGoldEarnedMeta = 0;

	/** Merchant stock refresh counter (every 3 runs). */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta|Nexus")
	int32 MerchantRestockRunCounter = 0;

	/** Best 1-based floor and kill count in a single run (Chronicler / highs). v4+ */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta|Nexus")
	int32 BestFloorReached = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta|Nexus")
	int32 BestKillsInRun = 0;

	/** Last captured run snapshot for resume-after-crash (see @ref UDFWorldTransitionSubsystem::SaveCheckpoint). */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta|Run")
	FDFRunState LastCheckpoint;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "DF|Meta|Run")
	ECheckpointType LastCheckpointType = ECheckpointType::RunStart;

	// --- Per-profile main menu / run preview (v5) ---

	/** Set false after first time the player is welcomed in Nexus. */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "DF|Profile")
	bool bIsFirstLaunch = true;

	/** True if a roguelike run was started and not yet cleared by victory/reset. */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "DF|Profile|Run")
	bool bHasActiveRun = false;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "DF|Profile|Run")
	FName LastRunClass = NAME_None;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "DF|Profile|Run")
	int32 LastRunFloor = 0;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "DF|Profile")
	FDateTime LastPlayedDate = FDateTime(0);

	/** Stamped when saved; use @c IsCompatible with project version. */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "DF|Profile")
	FString GameVersion = TEXT("0.1.0");

	/** 0, 1, or 2 for multi-slot; -1 = legacy / unspecified. */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "DF|Profile")
	int32 SlotIndex = -1;

	/** Run progress only: clears checkpoint and active run flags; keeps meta, unlocks, high score. */
	UFUNCTION(BlueprintCallable, Category = "DF|Profile|Save")
	void ResetRunData();

	/** True if this save is safe to load (same @c GameVersion family). */
	UFUNCTION(BlueprintCallable, Category = "DF|Profile|Save")
	bool IsCompatible() const;

	/** Profile slot file name. */
	UFUNCTION(BlueprintCallable, Category = "DF|Profile|Save", meta = (ReturnDisplayName = "SlotName"))
	static FName GetProfileSlotFName(int32 Index);

	/** Synchronous load from a profile index (0–2), or a new object if missing. */
	UFUNCTION(BlueprintCallable, Category = "DF|Profile|Save")
	static UDFSaveGame* LoadProfile(int32 ProfileIndex);

	/** Write to a profile index. */
	UFUNCTION(BlueprintCallable, Category = "DF|Profile|Save")
	static bool SaveProfile(UDFSaveGame* Data, int32 ProfileIndex);

	/** Synchronous load from the primary meta slot, or a new object if missing / invalid. */
	UFUNCTION(BlueprintCallable, Category = "DF|Meta|Save")
	static UDFSaveGame* Load();

	/** Synchronous write to the primary meta slot. */
	UFUNCTION(BlueprintCallable, Category = "DF|Meta|Save")
	static bool Save(UDFSaveGame* Data);

	static const TCHAR* GetSlotName() { return TEXT("DFMetaSave"); }
	static constexpr int32 UserIndex = 0;

	static const TCHAR* GetProfileSlotNameBase() { return TEXT("DungeonForged_Slot"); }
};
