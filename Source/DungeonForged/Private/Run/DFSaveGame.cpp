// Source/DungeonForged/Private/Run/DFSaveGame.cpp

#include "Run/DFSaveGame.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr int32 GDFSaveLatestVersion = 2;
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
			if (CastSave->SaveVersion < GDFSaveLatestVersion)
			{
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
