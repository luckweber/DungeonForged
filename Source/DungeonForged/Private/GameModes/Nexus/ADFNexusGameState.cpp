// Source/DungeonForged/Private/GameModes/Nexus/ADFNexusGameState.cpp
#include "GameModes/Nexus/ADFNexusGameState.h"
#include "Engine/DataTable.h"
#include "GameModes/Nexus/DFNexusLevelData.h"
#include "GameModes/Nexus/DFNexusTypes.h"
#include "Run/DFSaveGame.h"
#include "GameModes/Nexus/ADFNexusGameMode.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
	const FDFNexusLevelRow* FindLevelRowByNexusLevel(
		const UDataTable* const Table,
		const int32 InNexusLevel,
		FName& OutRowName)
	{
		OutRowName = NAME_None;
		if (!Table)
		{
			return nullptr;
		}
		for (const TPair<FName, uint8*>& RowPair : Table->GetRowMap())
		{
			if (const FDFNexusLevelRow* const R = Table->FindRow<FDFNexusLevelRow>(RowPair.Key, TEXT("NexusLevelRowLookup")))
			{
				if (R->NexusLevel == InNexusLevel)
				{
					OutRowName = RowPair.Key;
					return R;
				}
			}
		}
		return nullptr;
	}
}

ADFNexusGameState::ADFNexusGameState()
{
}

void ADFNexusGameState::ApplyFromSave(UDFSaveGame* const Save)
{
	if (!Save)
	{
		return;
	}
	TotalRunsCompleted = Save->TotalRuns;
	TotalRunsWon = Save->TotalWins;
	MetaXP = Save->MetaXP;
	MetaLevel = FMath::Max(1, Save->MetaLevel);
	UnlockedClasses = Save->UnlockedClasses;
	UnlockedNPCs = Save->UnlockedNPCs;
	CompletedUpgrades = Save->CompletedUpgrades;
}

void ADFNexusGameState::AddMetaXP(const int32 Amount, UDFSaveGame* const SaveToUpdate)
{
	if (Amount <= 0 || !SaveToUpdate)
	{
		return;
	}
	MetaXP = SaveToUpdate->MetaXP + Amount;
	SaveToUpdate->MetaXP = MetaXP;
	CheckMetaLevelUp(SaveToUpdate);
	UDFSaveGame::Save(SaveToUpdate);
	if (SaveToUpdate->PendingUnlocks.Num() > 0)
	{
		if (UWorld* const W = GetWorld())
		{
			if (ADFNexusGameMode* const GM = W->GetAuthGameMode<ADFNexusGameMode>())
			{
				GM->ProcessPendingUnlocksFromSave(SaveToUpdate, W->GetFirstPlayerController());
			}
		}
	}
}

void ADFNexusGameState::CheckMetaLevelUp(UDFSaveGame* const SaveToUpdate)
{
	if (!SaveToUpdate || !NexusLevelsTable)
	{
		return;
	}
	bool bLeveled = false;
	for (;;)
	{
		FName RowName;
		const FDFNexusLevelRow* const Row = FindLevelRowByNexusLevel(NexusLevelsTable, MetaLevel + 1, RowName);
		if (!Row || MetaXP < Row->MetaXPRequired)
		{
			break;
		}
		++MetaLevel;
		SaveToUpdate->MetaLevel = MetaLevel;
		bLeveled = true;
		if (!Row->UnlockClassRow.IsNone())
		{
			FDFPendingUnlockEntry E;
			E.Type = ENexusPendingUnlockType::UnlockClass;
			E.ClassRow = Row->UnlockClassRow;
			SaveToUpdate->PendingUnlocks.Add(E);
		}
		if (!Row->UnlockNPCRow.IsNone())
		{
			FDFPendingUnlockEntry E;
			E.Type = ENexusPendingUnlockType::UnlockNPC;
			E.NPCId = Row->UnlockNPCRow;
			SaveToUpdate->PendingUnlocks.Add(E);
		}
		for (const FName U : Row->UnlockUpgradeRows)
		{
			if (U.IsNone())
			{
				continue;
			}
			FDFPendingUnlockEntry E;
			E.Type = ENexusPendingUnlockType::UnlockUpgrade;
			E.UpgradeRow = U;
			SaveToUpdate->PendingUnlocks.Add(E);
		}
	}
	if (bLeveled)
	{
		OnMetaLevelUp.Broadcast(MetaLevel, MetaXP);
	}
}

int32 ADFNexusGameState::GetMetaXPToNextLevel() const
{
	if (!NexusLevelsTable)
	{
		return 0;
	}
	FName RName;
	if (const FDFNexusLevelRow* const Row = FindLevelRowByNexusLevel(NexusLevelsTable, MetaLevel + 1, RName))
	{
		return FMath::Max(0, Row->MetaXPRequired - MetaXP);
	}
	return 0;
}

float ADFNexusGameState::GetNexusXPFillRatio() const
{
	if (!NexusLevelsTable)
	{
		return 1.f;
	}
	FName A;
	FName B;
	const FDFNexusLevelRow* const RowMin = FindLevelRowByNexusLevel(NexusLevelsTable, MetaLevel, A);
	const FDFNexusLevelRow* const RowMax = FindLevelRowByNexusLevel(NexusLevelsTable, MetaLevel + 1, B);
	if (!RowMax)
	{
		return 1.f;
	}
	const int32 X0 = RowMin ? RowMin->MetaXPRequired : 0;
	const int32 X1 = RowMax->MetaXPRequired;
	if (X1 <= X0)
	{
		return 1.f;
	}
	return FMath::Clamp((float)(MetaXP - X0) / (float)(X1 - X0), 0.f, 1.f);
}
