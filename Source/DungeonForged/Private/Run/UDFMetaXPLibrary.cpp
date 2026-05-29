// Source/DungeonForged/Private/Run/UDFMetaXPLibrary.cpp
#include "Run/UDFMetaXPLibrary.h"
#include "Run/DFMetaRewardData.h"
#include "GameModes/Run/DFRunTypes.h"
#include "Settings/UDFRunDeveloperSettings.h"

namespace
{
UDataTable* GDefaultMetaXPRewardsTable = nullptr;

void SeedDefaultRow(UDataTable* const Table, const FName RowName, const ETravelReason Outcome, const int32 BaseXP,
	const int32 XPPerFloor, const int32 XPPerKill)
{
	FDFMetaXPRewardRow Row;
	Row.Outcome = Outcome;
	Row.BaseXP = BaseXP;
	Row.XPPerFloor = XPPerFloor;
	Row.XPPerKill = XPPerKill;
	Table->AddRow(RowName, Row);
}

UDataTable* BuildDefaultMetaXPRewardsTable()
{
	if (GDefaultMetaXPRewardsTable)
	{
		return GDefaultMetaXPRewardsTable;
	}
	GDefaultMetaXPRewardsTable = NewObject<UDataTable>(GetTransientPackage(), TEXT("DFDefaultMetaXPRewards"));
	GDefaultMetaXPRewardsTable->RowStruct = FDFMetaXPRewardRow::StaticStruct();
	SeedDefaultRow(GDefaultMetaXPRewardsTable, TEXT("Victory"), ETravelReason::Victory, 500, 50, 2);
	SeedDefaultRow(GDefaultMetaXPRewardsTable, TEXT("Defeat"), ETravelReason::Defeat, 100, 20, 1);
	SeedDefaultRow(GDefaultMetaXPRewardsTable, TEXT("AbandonRun"), ETravelReason::AbandonRun, 25, 5, 0);
	return GDefaultMetaXPRewardsTable;
}
} // namespace

UDataTable* UDFMetaXPLibrary::GetMetaXPRewardsTable()
{
	if (const UDFRunDeveloperSettings* const Dev = GetDefault<UDFRunDeveloperSettings>())
	{
		if (!Dev->MetaXPRewardsTable.IsNull())
		{
			if (UDataTable* const Loaded = Dev->MetaXPRewardsTable.LoadSynchronous())
			{
				return Loaded;
			}
		}
	}
	return BuildDefaultMetaXPRewardsTable();
}

bool UDFMetaXPLibrary::FindRewardRowForOutcome(const ETravelReason Outcome, FDFMetaXPRewardRow& OutRow)
{
	if (UDataTable* const Table = GetMetaXPRewardsTable())
	{
		for (TPair<FName, uint8*> const& Pair : Table->GetRowMap())
		{
			if (FDFMetaXPRewardRow* const Row = Table->FindRow<FDFMetaXPRewardRow>(Pair.Key, TEXT("FindRewardRowForOutcome"), false))
			{
				if (Row->Outcome == Outcome)
				{
					OutRow = *Row;
					return true;
				}
			}
		}
	}
	return false;
}

int32 UDFMetaXPLibrary::CalculateMetaXPGain(const ETravelReason Outcome, const FDFRunSummary& Summary)
{
	FDFMetaXPRewardRow Row;
	if (FindRewardRowForOutcome(Outcome, Row))
	{
		return FMath::Max(0, Row.BaseXP + Summary.FloorReached * Row.XPPerFloor + Summary.Kills * Row.XPPerKill);
	}
	return 0;
}
