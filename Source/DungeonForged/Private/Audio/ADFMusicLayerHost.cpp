// Source/DungeonForged/Private/Audio/ADFMusicLayerHost.cpp
#include "Audio/ADFMusicLayerHost.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"

ADFMusicLayerHost::ADFMusicLayerHost()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	AudioRoot = CreateDefaultSubobject<USceneComponent>(TEXT("AudioRoot"));
	SetRootComponent(AudioRoot);

	MusicLayerBase = CreateDefaultSubobject<UAudioComponent>(TEXT("MusicLayerBase"));
	MusicLayerBase->SetupAttachment(AudioRoot);
	MusicLayerBase->bAutoActivate = false;
	MusicLayerBase->bAllowSpatialization = false;
	MusicLayerBase->bIsUISound = false;

	MusicLayerCombat = CreateDefaultSubobject<UAudioComponent>(TEXT("MusicLayerCombat"));
	MusicLayerCombat->SetupAttachment(AudioRoot);
	MusicLayerCombat->bAutoActivate = false;
	MusicLayerCombat->bAllowSpatialization = false;
	MusicLayerCombat->bIsUISound = false;

	MusicLayerBoss = CreateDefaultSubobject<UAudioComponent>(TEXT("MusicLayerBoss"));
	MusicLayerBoss->SetupAttachment(AudioRoot);
	MusicLayerBoss->bAutoActivate = false;
	MusicLayerBoss->bAllowSpatialization = false;
	MusicLayerBoss->bIsUISound = false;
}
