// Source/DungeonForged/Private/GameModes/MainMenu/ADFMainMenuGameMode.cpp
#include "GameModes/MainMenu/ADFMainMenuGameMode.h"
#include "GameModes/MainMenu/ADFMainMenuHUD.h"
#include "Run/UDFSaveSlotManagerSubsystem.h"
#include "Audio/UDFMusicManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "GameFramework/SpectatorPawn.h"

ADFMainMenuGameMode::ADFMainMenuGameMode()
{
	HUDClass = ADFMainMenuHUD::StaticClass();
	bUseSeamlessTravel = false;
	DefaultPawnClass = ASpectatorPawn::StaticClass();
	SpectatorClass = ASpectatorPawn::StaticClass();
	// Menu map: do not use PlayerController with movement pawn — spectator only.
	// Still needs a Pawn to receive camera/sequence binding if the designer binds a camera in Sequencer to it.
}

void ADFMainMenuGameMode::StartPlay()
{
	Super::StartPlay();
	PlayBackgroundSequence();
}

void ADFMainMenuGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFSaveSlotManagerSubsystem* const Slots = GI->GetSubsystem<UDFSaveSlotManagerSubsystem>())
		{
			Slots->InitializeOrMigrateSlots();
		}
	}
	if (UWorld* const W = GetWorld())
	{
		if (W->GetNetMode() != NM_DedicatedServer)
		{
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
	if (!BackgroundLoopSequence)
	{
		return;
	}
	UWorld* const W = GetWorld();
	if (!W || W->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (BgSequencePlayer)
	{
		return;
	}
	FMovieSceneSequencePlaybackSettings Settings;
	ALevelSequenceActor* LSA = nullptr;
	BgSequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
		W, BackgroundLoopSequence, Settings, LSA);
	BgSequenceActor = LSA;
	if (BgSequencePlayer)
	{
		BgSequencePlayer->OnFinished.AddDynamic(this, &ADFMainMenuGameMode::OnMainMenuSequenceFinished);
		BgSequencePlayer->Play();
	}
}

void ADFMainMenuGameMode::OnMainMenuSequenceFinished()
{
	if (BgSequencePlayer)
	{
		BgSequencePlayer->Play();
	}
}

void ADFMainMenuGameMode::StopBackgroundSequence()
{
	if (BgSequencePlayer)
	{
		BgSequencePlayer->OnFinished.RemoveAll(this);
		BgSequencePlayer->Stop();
	}
	BgSequencePlayer = nullptr;
	BgSequenceActor = nullptr;
}
