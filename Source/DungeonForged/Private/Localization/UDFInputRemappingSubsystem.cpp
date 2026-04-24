// Source/DungeonForged/Private/Localization/UDFInputRemappingSubsystem.cpp
#include "Localization/UDFInputRemappingSubsystem.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "EnhancedInputSubsystems.h"
#include "Run/DFSaveGame.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "GameplayTagContainer.h"

void UDFInputRemappingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadRemapping();
}

void UDFInputRemappingSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

UEnhancedInputLocalPlayerSubsystem* UDFInputRemappingSubsystem::GetEISS(ULocalPlayer* const LocalPlayer)
{
	return LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
}

bool UDFInputRemappingSubsystem::RegisterInputMappingContextForLocalPlayer(
	ULocalPlayer* const LocalPlayer,
	UInputMappingContext* IMC)
{
	if (!LocalPlayer)
	{
		return false;
	}
	UInputMappingContext* const UseIMC = IMC ? IMC : CurrentIMC.Get();
	if (!UseIMC)
	{
		return false;
	}
	if (UEnhancedInputLocalPlayerSubsystem* const EISS = GetEISS(LocalPlayer))
	{
		if (UEnhancedInputUserSettings* const US = EISS->GetUserSettings())
		{
			US->RegisterInputMappingContext(UseIMC);
			CurrentIMC = UseIMC;
			ApplyAllSavedToUserSettings(LocalPlayer);
			return true;
		}
	}
	return false;
}

void UDFInputRemappingSubsystem::LoadRemapping()
{
	RemappedKeys.Empty();
	if (const UDFSaveGame* S = UDFSaveGame::Load())
	{
		for (const TPair<FName, FString>& P : S->SavedKeyBindings)
		{
			RemappedKeys.Add(P.Key, FKey(FName(*P.Value)));
		}
	}
}

void UDFInputRemappingSubsystem::SaveRemapping()
{
	if (UDFSaveGame* S = UDFSaveGame::Load())
	{
		S->SavedKeyBindings.Empty();
		for (const TPair<FName, FKey>& P : RemappedKeys)
		{
			S->SavedKeyBindings.Add(P.Key, P.Value.GetFName().ToString());
		}
		UDFSaveGame::Save(S);
	}
}

void UDFInputRemappingSubsystem::ApplyAllSavedToUserSettings(ULocalPlayer* const LocalPlayer)
{
	UEnhancedInputUserSettings* const US = GetEISS(LocalPlayer) ? GetEISS(LocalPlayer)->GetUserSettings() : nullptr;
	if (!US)
	{
		return;
	}
	for (const TPair<FName, FKey>& P : RemappedKeys)
	{
		FMapPlayerKeyArgs Args;
		Args.MappingName = P.Key;
		Args.Slot = EPlayerMappableKeySlot::First;
		Args.NewKey = P.Value;
		FGameplayTagContainer Fail;
		US->MapPlayerKey(Args, Fail);
	}
}

void UDFInputRemappingSubsystem::RemapKey(
	ULocalPlayer* const LocalPlayer,
	const FName InputActionOrMappingName,
	const FKey NewKey)
{
	UEnhancedInputLocalPlayerSubsystem* const EISS = GetEISS(LocalPlayer);
	UEnhancedInputUserSettings* const US = EISS ? EISS->GetUserSettings() : nullptr;
	if (!US)
	{
		return;
	}
	FMapPlayerKeyArgs Args;
	Args.MappingName = InputActionOrMappingName;
	Args.Slot = EPlayerMappableKeySlot::First;
	Args.NewKey = NewKey;
	FGameplayTagContainer Fail;
	US->MapPlayerKey(Args, Fail);
	RemappedKeys.Add(InputActionOrMappingName, NewKey);
	SaveRemapping();
}

void UDFInputRemappingSubsystem::ResetToDefaults(ULocalPlayer* const LocalPlayer)
{
	UEnhancedInputLocalPlayerSubsystem* const EISS = GetEISS(LocalPlayer);
	UEnhancedInputUserSettings* const US = EISS ? EISS->GetUserSettings() : nullptr;
	if (!US)
	{
		return;
	}
	FGameplayTagContainer Fail;
	US->ResetKeyProfileToDefault(US->GetCurrentKeyProfileIdentifier(), Fail);
	RemappedKeys.Empty();
	if (UDFSaveGame* S = UDFSaveGame::Load())
	{
		S->SavedKeyBindings.Empty();
		UDFSaveGame::Save(S);
	}
}
