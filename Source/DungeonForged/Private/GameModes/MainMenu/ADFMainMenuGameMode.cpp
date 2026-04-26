// Source/DungeonForged/Private/GameModes/MainMenu/ADFMainMenuGameMode.cpp
#include "GameModes/MainMenu/ADFMainMenuGameMode.h"
#include "Run/UDFSaveSlotManagerSubsystem.h"
#include "Audio/UDFMusicManagerSubsystem.h"
#include "World/UDFCinematicSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/SpectatorPawn.h"

ADFMainMenuGameMode::ADFMainMenuGameMode()
{
	MainMenuHUDClass = TSubclassOf<ADFMainMenuHUD>(ADFMainMenuHUD::StaticClass());
	HUDClass = MainMenuHUDClass;
	bUseSeamlessTravel = false;	DefaultPawnClass = ASpectatorPawn::StaticClass();
	SpectatorClass = ASpectatorPawn::StaticClass();
	// Menu map: do not use PlayerController with movement pawn — spectator only.
	// Still needs a Pawn to receive camera/sequence binding if the designer binds a camera in Sequencer to it.
}

void ADFMainMenuGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	bPlayerHasProfileSaveData = false;
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFSaveSlotManagerSubsystem* const Slots = GI->GetSubsystem<UDFSaveSlotManagerSubsystem>())
		{
			Slots->InitializeOrMigrateSlots();
			bPlayerHasProfileSaveData = Slots->HasAnyProfileOrLegacySave();
		}
	}
	if (MainMenuHUDClass)
	{
		HUDClass = MainMenuHUDClass;
	}	if (UWorld* const W = GetWorld())
	{
		if (W->GetNetMode() != NM_DedicatedServer)
		{
			if (BackgroundLoopSequence)
			{
				if (UDFCinematicSubsystem* const Cine = W->GetSubsystem<UDFCinematicSubsystem>())
				{
					Cine->PlayLooping(BackgroundLoopSequence);
				}
			}
			if (UDFMusicManagerSubsystem* const M = W->GetSubsystem<UDFMusicManagerSubsystem>())
			{
				M->SetMusicState(EMusicState::MainMenu);
			}
		}
	}
}

void ADFMainMenuGameMode::PostLogin(APlayerController* const NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (GetNetMode() == NM_Client)
	{
		return;
	}
	if (!NewPlayer || !NewPlayer->IsLocalController())
	{
		return;
	}
	if (ADFMainMenuHUD* const H = Cast<ADFMainMenuHUD>(NewPlayer->GetHUD()))
	{
		H->OnLocalPlayerMenuReady(NewPlayer);
	}
}

void ADFMainMenuGameMode::PlayBackgroundSequence()
{
	if (UWorld* const W = GetWorld())
	{
		if (UDFCinematicSubsystem* const Cine = W->GetSubsystem<UDFCinematicSubsystem>())
		{
			Cine->PlayLooping(BackgroundLoopSequence);
		}
	}
}

void ADFMainMenuGameMode::StopBackgroundSequence()
{
	if (UWorld* const W = GetWorld())
	{
		if (UDFCinematicSubsystem* const Cine = W->GetSubsystem<UDFCinematicSubsystem>())
		{
			Cine->Stop();
		}
	}
}
