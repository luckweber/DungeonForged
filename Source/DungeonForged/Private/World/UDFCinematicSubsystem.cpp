// Source/DungeonForged/Private/World/UDFCinematicSubsystem.cpp
#include "World/UDFCinematicSubsystem.h"
#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "Engine/World.h"

void UDFCinematicSubsystem::Deinitialize()
{
	Stop();
	Super::Deinitialize();
}

void UDFCinematicSubsystem::PlayLooping(ULevelSequence* const Sequence)
{
	Stop();
	if (!Sequence)
	{
		return;
	}
	UWorld* const W = GetWorld();
	if (!W || W->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	ActiveSequence = Sequence;
	FMovieSceneSequencePlaybackSettings Settings;
	ALevelSequenceActor* LSA = nullptr;
	SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(W, Sequence, Settings, LSA);
	SequenceActor = LSA;
	if (SequencePlayer)
	{
		SequencePlayer->OnFinished.AddDynamic(this, &UDFCinematicSubsystem::OnSequenceFinished);
		SequencePlayer->Play();
	}
}

void UDFCinematicSubsystem::OnSequenceFinished()
{
	if (SequencePlayer && ActiveSequence)
	{
		SequencePlayer->Play();
	}
}

void UDFCinematicSubsystem::Stop()
{
	if (SequencePlayer)
	{
		SequencePlayer->OnFinished.RemoveAll(this);
		SequencePlayer->Stop();
	}
	SequencePlayer = nullptr;
	SequenceActor = nullptr;
	ActiveSequence = nullptr;
}
