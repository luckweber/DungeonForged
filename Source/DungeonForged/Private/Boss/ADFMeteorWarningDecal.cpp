// Source/DungeonForged/Private/Boss/ADFMeteorWarningDecal.cpp
#include "Boss/ADFMeteorWarningDecal.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"

ADFMeteorWarningDecal::ADFMeteorWarningDecal()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	GroundDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("Decal"));
	GroundDecal->SetupAttachment(Root);
	GroundDecal->DecalSize = FVector(DecalRadius * 2.f, DecalRadius * 2.f, 800.f);
}

void ADFMeteorWarningDecal::BeginPlay()
{
	Super::BeginPlay();
	Elapsed = 0.f;
	if (GroundDecal)
	{
		GroundDecal->DecalSize = FVector(DecalRadius * 2.f, DecalRadius * 2.f, 800.f);
	}
	GroundDecal->SetWorldLocation(GetActorLocation() - FVector(0.f, 0.f, 10.f));
	if (UWorld* W = GetWorld())
	{
		if (RumbleLoop)
		{
			UGameplayStatics::PlaySoundAtLocation(this, RumbleLoop, GetActorLocation());
		}
	}
	if (GroundDecal && GroundDecal->GetDecalMaterial())
	{
		DecalMid = GroundDecal->CreateDynamicMaterialInstance();
	}
}

void ADFMeteorWarningDecal::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;
	if (DecalMid && PulseMaterialParamName != NAME_None)
	{
		const float A = 0.5f + 0.5f * FMath::Sin(Elapsed * 3.14159f * PulseRate);
		DecalMid->SetScalarParameterValue(PulseMaterialParamName, A);
	}
}
