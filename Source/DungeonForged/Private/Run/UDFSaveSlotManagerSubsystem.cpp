// Source/DungeonForged/Private/Run/UDFSaveSlotManagerSubsystem.cpp

#include "Run/UDFSaveSlotManagerSubsystem.h"
#include "Run/DFSaveGame.h"
#include "Run/DFRunManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** @c UDFRunManager / @c ADFNexusGameMode still use @c UDFSaveGame::Load; mirror the active profile. */
	void MirrorToLegacy(UDFSaveGame* const Save)
	{
		if (Save)
		{
			UDFSaveGame::Save(Save);
		}
	}
}

void UDFSaveSlotManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadAllSlotHeaders();
}

void UDFSaveSlotManagerSubsystem::InitializeOrMigrateSlots()
{
	LoadAllSlotHeaders();
	// If no profile data but legacy global meta save exists, copy into profile 0 once.
	if (!HasAnyProfileOrLegacySave())
	{
		return;
	}
	bool bAnyProfile = false;
	for (int32 I = 0; I < MaxSlots; ++I)
	{
		if (LoadedSlots.IsValidIndex(I) && IsValid(LoadedSlots[I].Get()))
		{
			bAnyProfile = true;
			break;
		}
	}
	if (!bAnyProfile && UGameplayStatics::DoesSaveGameExist(UDFSaveGame::GetSlotName(), UDFSaveGame::UserIndex))
	{
		UDFSaveGame* const Legacy = UDFSaveGame::Load();
		if (Legacy)
		{
			UDFSaveGame::SaveProfile(Legacy, 0);
			LoadAllSlotHeaders();
		}
	}
}

void UDFSaveSlotManagerSubsystem::LoadAllSlotHeaders()
{
	LoadedSlots.SetNum(MaxSlots);
	for (int32 I = 0; I < MaxSlots; ++I)
	{
		const FString Slot = UDFSaveGame::GetProfileSlotFName(I).ToString();
		if (UGameplayStatics::DoesSaveGameExist(Slot, UDFSaveGame::UserIndex))
		{
			USaveGame* const Loaded = UGameplayStatics::LoadGameFromSlot(Slot, UDFSaveGame::UserIndex);
			if (UDFSaveGame* const CastSave = Cast<UDFSaveGame>(Loaded))
			{
				CastSave->SlotIndex = I;
				LoadedSlots[I] = CastSave;
			}
			else
			{
				LoadedSlots[I] = nullptr;
				const FString Reason = TEXT("Deserializacao invalida");
				OnSlotLoadFailed.Broadcast(I, Reason);
			}
		}
		else
		{
			LoadedSlots[I] = nullptr;
		}
		// Do not broadcast from here: listeners (e.g. save slot UI) may call
		// LoadAllSlotHeaders again and recurse until stack overflow. Mutations
		// (Select, Save, Delete) broadcast separately after cache is already updated.
	}
}

UDFSaveGame* UDFSaveSlotManagerSubsystem::GetSlotData(int32 const SlotIndex) const
{
	if (LoadedSlots.IsValidIndex(SlotIndex))
	{
		return LoadedSlots[SlotIndex].Get();
	}
	return nullptr;
}

UDFSaveGame* UDFSaveSlotManagerSubsystem::GetActiveSave() const
{
	if (ActiveSlotIndex >= 0 && ActiveSlotIndex < MaxSlots)
	{
		return LoadedSlots.IsValidIndex(ActiveSlotIndex) ? LoadedSlots[ActiveSlotIndex].Get() : nullptr;
	}
	return nullptr;
}

void UDFSaveSlotManagerSubsystem::SelectSlot(int32 const SlotIndex)
{
	const int32 I = FMath::Clamp(SlotIndex, 0, MaxSlots - 1);
	LoadedSlots[I] = UDFSaveGame::LoadProfile(I);
	ActiveSlotIndex = I;
	MirrorToLegacy(GetActiveSave());
	BroadcastSlotChanged(I);
}

bool UDFSaveSlotManagerSubsystem::SaveActiveSlot()
{
	if (ActiveSlotIndex < 0)
	{
		return false;
	}
	UDFSaveGame* const S = GetActiveSave();
	if (!S)
	{
		return false;
	}
	if (!UDFSaveGame::SaveProfile(S, ActiveSlotIndex))
	{
		return false;
	}
	MirrorToLegacy(S);
	BroadcastSlotChanged(ActiveSlotIndex);
	return true;
}

bool UDFSaveSlotManagerSubsystem::SaveActiveSlotWithBackup()
{
	if (ActiveSlotIndex < 0)
	{
		return SaveActiveSlot();
	}
	UDFSaveGame* const S = GetActiveSave();
	if (!S)
	{
		return false;
	}
	const FString BackupSlot = FString::Printf(
		TEXT("%s_Backup"), *UDFSaveGame::GetProfileSlotFName(ActiveSlotIndex).ToString());
	UGameplayStatics::SaveGameToSlot(S, BackupSlot, UDFSaveGame::UserIndex);
	return SaveActiveSlot();
}

void UDFSaveSlotManagerSubsystem::DeleteSlot(int32 const SlotIndex)
{
	const int32 I = FMath::Clamp(SlotIndex, 0, MaxSlots - 1);
	UGameplayStatics::DeleteGameInSlot(UDFSaveGame::GetProfileSlotFName(I).ToString(), UDFSaveGame::UserIndex);
	LoadedSlots[I] = nullptr;
	if (ActiveSlotIndex == I)
	{
		ActiveSlotIndex = -1;
	}
	// Legacy mirror: no longer valid if we had only that slot; caller may refresh
	OnSlotChanged.Broadcast(I);
	OnSlotDeleted.Broadcast(I);
}

void UDFSaveSlotManagerSubsystem::BroadcastSlotChanged(int32 const SlotIndex)
{
	OnSlotChanged.Broadcast(SlotIndex);
}

bool UDFSaveSlotManagerSubsystem::IsSlotEmpty(int32 const SlotIndex) const
{
	if (SlotIndex < 0 || SlotIndex >= MaxSlots)
	{
		return true;
	}
	return !UGameplayStatics::DoesSaveGameExist(UDFSaveGame::GetProfileSlotFName(SlotIndex).ToString(), UDFSaveGame::UserIndex);
}

bool UDFSaveSlotManagerSubsystem::HasAnyProfileOrLegacySave() const
{
	for (int32 I = 0; I < MaxSlots; ++I)
	{
		if (UGameplayStatics::DoesSaveGameExist(UDFSaveGame::GetProfileSlotFName(I).ToString(), UDFSaveGame::UserIndex))
		{
			return true;
		}
	}
	if (UGameplayStatics::DoesSaveGameExist(UDFSaveGame::GetSlotName(), UDFSaveGame::UserIndex))
	{
		return true;
	}
	return false;
}

UDFSaveGame* UDFSaveSlotManagerSubsystem::ResolveMutableMetaSave()
{
	if (UDFSaveGame* const Active = GetActiveSave())
	{
		return Active;
	}
	InitializeOrMigrateSlots();
	for (int32 I = 0; I < MaxSlots; ++I)
	{
		if (GetSlotData(I))
		{
			SelectSlot(I);
			return GetActiveSave();
		}
	}
	SelectSlot(0);
	return GetActiveSave();
}

UDFSaveGame* UDFSaveSlotManagerSubsystem::GetActiveOrLegacyMetaSave()
{
	return ResolveMutableMetaSave();
}

bool UDFSaveSlotManagerSubsystem::FindSlotWithActiveRun(int32& OutSlotIndex) const
{
	auto TrySlot = [this](const int32 SlotIdx) -> bool
	{
		if (UDFSaveGame* const Save = GetSlotData(SlotIdx))
		{
			if (UDFRunManager::CanResumeFromSave(Save))
			{
				return true;
			}
		}
		return false;
	};

	if (ActiveSlotIndex >= 0 && TrySlot(ActiveSlotIndex))
	{
		OutSlotIndex = ActiveSlotIndex;
		return true;
	}
	for (int32 I = 0; I < MaxSlots; ++I)
	{
		if (I == ActiveSlotIndex)
		{
			continue;
		}
		if (TrySlot(I))
		{
			OutSlotIndex = I;
			return true;
		}
	}
	OutSlotIndex = INDEX_NONE;
	return false;
}
