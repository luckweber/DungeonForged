// Source/DungeonForged/Private/Run/DFSaveGame.cpp

#include "Run/DFSaveGame.h"
#include "Run/DFRunManager.h"
#include "World/DFWorldTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/DateTime.h"
#include "Engine/DataTable.h"
#include "GameModes/Nexus/DFNexusLevelData.h"

namespace
{
	constexpr int32 GDFSaveLatestVersion = 6;

	const FDFNexusLevelRow* FindLevelRowByNexusLevel(
		UDataTable const* const Table,
		int32 const InNexusLevel,
		FName& OutRowName)
	{
		OutRowName = NAME_None;
		if (!Table)
		{
			return nullptr;
		}
		for (TPair<FName, uint8*> const& RowPair : Table->GetRowMap())
		{
			if (FDFNexusLevelRow const* const R =
					Table->FindRow<FDFNexusLevelRow>(RowPair.Key, TEXT("SaveGameNexusLevelLookup"), false))
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

	void ApplyMigrations(UDFSaveGame* const CastSave)
	{
		if (CastSave->SaveVersion < 2)
		{
			CastSave->PreferredLanguage = EDFLanguage::PortugueseBrazil;
			CastSave->PreferredCultureCode = TEXT("pt-BR");
			CastSave->AccessibilitySettings = FDFAccessibilitySettings();
			CastSave->SavedKeyBindings.Empty();
		}
		if (CastSave->SaveVersion < 4)
		{
			CastSave->LastCheckpoint = FDFRunState();
			CastSave->LastCheckpointType = ECheckpointType::RunStart;
			CastSave->BestFloorReached = 0;
			CastSave->BestKillsInRun = 0;
		}
		if (CastSave->SaveVersion < 5)
		{
			if (CastSave->GameVersion.IsEmpty())
			{
				CastSave->GameVersion = TEXT("0.1.0");
			}
		}
		if (CastSave->SaveVersion < 6)
		{
			CastSave->bLastRunWasVictory = false;
		}
		CastSave->SaveVersion = GDFSaveLatestVersion;
	}
}

FName UDFSaveGame::GetProfileSlotFName(int32 const Index)
{
	return FName(*FString::Printf(TEXT("DungeonForged_Slot%d"), FMath::Clamp(Index, 0, 2)));
}

UDFSaveGame* UDFSaveGame::LoadProfile(int32 const ProfileIndex)
{
	const int32 I = FMath::Clamp(ProfileIndex, 0, 2);
	const FString SlotStr = UDFSaveGame::GetProfileSlotFName(I).ToString();
	if (UGameplayStatics::DoesSaveGameExist(SlotStr, UDFSaveGame::UserIndex))
	{
		USaveGame* const Loaded = UGameplayStatics::LoadGameFromSlot(SlotStr, UDFSaveGame::UserIndex);
		if (UDFSaveGame* const CastSave = Cast<UDFSaveGame>(Loaded))
		{
			ApplyMigrations(CastSave);
			CastSave->SlotIndex = I;
			return CastSave;
		}
	}
	UDFSaveGame* const N = NewObject<UDFSaveGame>(GetTransientPackage(), UDFSaveGame::StaticClass());
	N->SlotIndex = I;
	N->SaveVersion = GDFSaveLatestVersion;
	return N;
}

bool UDFSaveGame::SaveProfile(UDFSaveGame* const Data, int32 const ProfileIndex)
{
	if (!Data)
	{
		return false;
	}
	const int32 I = FMath::Clamp(ProfileIndex, 0, 2);
	Data->SlotIndex = I;
	Data->LastPlayedDate = FDateTime::Now();
	return UGameplayStatics::SaveGameToSlot(Data, UDFSaveGame::GetProfileSlotFName(I).ToString(), UDFSaveGame::UserIndex);
}

void UDFSaveGame::ResetRunData()
{
	LastCheckpoint = FDFRunState();
	LastCheckpointType = ECheckpointType::RunStart;
	bHasActiveRun = false;
	LastRunClass = NAME_None;
	LastRunFloor = 0;
}

bool UDFSaveGame::IsCompatible() const
{
	if (GameVersion.IsEmpty())
	{
		return true;
	}
	FString ProjectVer;
	if (!GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectVersion"), ProjectVer, GGameIni))
	{
		return true;
	}
	return GameVersion == ProjectVer;
}

float UDFSaveGame::GetNexusMetaXPFillRatio(UDataTable const* const NexusLevelsTable) const
{
	if (!NexusLevelsTable)
	{
		const float Den = FMath::Max(1000.f, float(FMath::Max(1, MetaLevel) * 800));
		return FMath::Clamp(MetaXP / Den, 0.f, 1.f);
	}
	FName A;
	FName B;
	FDFNexusLevelRow const* const RowMin = FindLevelRowByNexusLevel(NexusLevelsTable, MetaLevel, A);
	FDFNexusLevelRow const* const RowMax = FindLevelRowByNexusLevel(NexusLevelsTable, MetaLevel + 1, B);
	if (!RowMax)
	{
		return 1.f;
	}
	int32 const X0 = RowMin ? RowMin->MetaXPRequired : 0;
	int32 const X1 = RowMax->MetaXPRequired;
	if (X1 <= X0)
	{
		return 1.f;
	}
	return FMath::Clamp(float(MetaXP - X0) / float(X1 - X0), 0.f, 1.f);
}

UDFSaveGame* UDFSaveGame::Load()
{
	if (UGameplayStatics::DoesSaveGameExist(UDFSaveGame::GetSlotName(), UDFSaveGame::UserIndex))
	{
		USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(UDFSaveGame::GetSlotName(), UDFSaveGame::UserIndex);
		if (UDFSaveGame* CastSave = Cast<UDFSaveGame>(Loaded))
		{
			ApplyMigrations(CastSave);
			return CastSave;
		}
	}
	return NewObject<UDFSaveGame>(GetTransientPackage(), UDFSaveGame::StaticClass());
}

bool UDFSaveGame::Save(UDFSaveGame* Data)
{
	if (!Data)
	{
		return false;
	}
	return UGameplayStatics::SaveGameToSlot(Data, UDFSaveGame::GetSlotName(), UDFSaveGame::UserIndex);
}
