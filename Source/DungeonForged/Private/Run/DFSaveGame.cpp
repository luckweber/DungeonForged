// Source/DungeonForged/Private/Run/DFSaveGame.cpp

#include "Run/DFSaveGame.h"
#include "Run/DFRunManager.h"
#include "World/DFWorldTypes.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr int32 GDFSaveLatestVersion = 4;
}

UDFSaveGame* UDFSaveGame::Load()
{
	if (UGameplayStatics::DoesSaveGameExist(UDFSaveGame::GetSlotName(), UDFSaveGame::UserIndex))
	{
		USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(UDFSaveGame::GetSlotName(), UDFSaveGame::UserIndex);
		if (UDFSaveGame* CastSave = Cast<UDFSaveGame>(Loaded))
		{
			if (CastSave->SaveVersion < 2)
			{
				CastSave->PreferredLanguage = EDFLanguage::PortugueseBrazil;
				CastSave->PreferredCultureCode = TEXT("pt-BR");
				CastSave->AccessibilitySettings = FDFAccessibilitySettings();
				// v2 default VoiceVolume; struct ctor covers new fields.
				CastSave->SavedKeyBindings.Empty();
			}
			// v3: Nexus meta (MetaXP, PendingUnlocks, RunHistory) — UPROPERTY defaults apply for new fields.
			// v4: LastCheckpoint, BestFloorReached, BestKillsInRun
			if (CastSave->SaveVersion < GDFSaveLatestVersion)
			{
				if (CastSave->SaveVersion < 4)
				{
					CastSave->LastCheckpoint = FDFRunState();
					CastSave->LastCheckpointType = ECheckpointType::RunStart;
					CastSave->BestFloorReached = 0;
					CastSave->BestKillsInRun = 0;
				}
				CastSave->SaveVersion = GDFSaveLatestVersion;
			}
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
