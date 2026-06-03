// Source/DungeonForged/Private/Run/UDFSaveLibrary.cpp
#include "Run/UDFSaveLibrary.h"
#include "Run/DFSaveGame.h"
#include "Run/UDFSaveSlotManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UDFSaveSlotManagerSubsystem* UDFSaveLibrary::GetSaveSlots(const UObject* const WorldContextObject)
{
	if (UWorld* const World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr)
	{
		if (UGameInstance* const GI = World->GetGameInstance())
		{
			return GI->GetSubsystem<UDFSaveSlotManagerSubsystem>();
		}
	}
	return nullptr;
}

UDFSaveGame* UDFSaveLibrary::ResolveMutableMetaSave(const UObject* const WorldContextObject)
{
	if (UDFSaveSlotManagerSubsystem* const Slots = GetSaveSlots(WorldContextObject))
	{
		return Slots->ResolveMutableMetaSave();
	}
	return nullptr;
}

UDFSaveGame* UDFSaveLibrary::GetMutableMetaSave(const UObject* const WorldContextObject)
{
	return ResolveMutableMetaSave(WorldContextObject);
}

const UDFSaveGame* UDFSaveLibrary::GetMetaSave(const UObject* const WorldContextObject)
{
	return ResolveMutableMetaSave(WorldContextObject);
}

bool UDFSaveLibrary::SaveMetaSave(const UObject* const WorldContextObject, UDFSaveGame* const SaveData)
{
	if (!SaveData)
	{
		return false;
	}
	if (UDFSaveSlotManagerSubsystem* const Slots = GetSaveSlots(WorldContextObject))
	{
		if (Slots->GetActiveSave() == SaveData)
		{
			return Slots->SaveActiveSlotWithBackup();
		}
		if (SaveData->SlotIndex >= 0 && SaveData->SlotIndex < UDFSaveSlotManagerSubsystem::MaxSlots)
		{
			if (UDFSaveGame::SaveProfile(SaveData, SaveData->SlotIndex))
			{
				Slots->BroadcastSlotChanged(SaveData->SlotIndex);
				return true;
			}
		}
		return Slots->SaveActiveSlotWithBackup();
	}
	return UDFSaveGame::Save(SaveData);
}
