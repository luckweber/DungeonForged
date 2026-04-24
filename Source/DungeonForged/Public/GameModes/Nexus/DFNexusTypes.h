// Source/DungeonForged/Public/GameModes/Nexus/DFNexusTypes.h
#pragma once

#include "CoreMinimal.h"
#include "DFNexusTypes.generated.h"

UENUM(BlueprintType)
enum class ENexusPendingUnlockType : uint8
{
	UnlockClass UMETA(DisplayName = "Class"),
	UnlockNPC UMETA(DisplayName = "NPC"),
	UnlockUpgrade UMETA(DisplayName = "Upgrade")
};

USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFPendingUnlockEntry
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Nexus")
	ENexusPendingUnlockType Type = ENexusPendingUnlockType::UnlockClass;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Nexus")
	FName ClassRow = NAME_None;

	/** e.g. matches @ref ADFNexusNPCBase::NPCId */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Nexus")
	FName NPCId = NAME_None;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Nexus")
	FName UpgradeRow = NAME_None;
};

UENUM(BlueprintType)
enum class EDFRunRecordOutcome : uint8
{
	None = 0,
	Abandoned = 1,
	Defeat = 2,
	Victory = 3
};

USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFRunHistoryEntry
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Nexus")
	int32 RunIndex = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Nexus")
	FName ClassName = NAME_None;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Nexus")
	int32 FloorReached = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Nexus")
	int32 Gold = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Nexus")
	int32 Score = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Nexus")
	float TimeSeconds = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Nexus")
	EDFRunRecordOutcome Outcome = EDFRunRecordOutcome::None;
};
