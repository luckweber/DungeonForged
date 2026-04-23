// Source/DungeonForged/Private/Combat/DFGroundTargetActor.cpp
#include "Combat/DFGroundTargetActor.h"
#include "Components/DecalComponent.h"

ADFGroundTargetActor::ADFGroundTargetActor()
{
	CollisionRadius = 800.f;
	CollisionHeight = 0.f;
	RangeDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("RangeDecal"));
	RangeDecal->SetupAttachment(RootComponent);
	RangeDecal->DecalSize = FVector(800.f, 1600.f, 1600.f);
	RangeDecal->SetWorldRotation(FRotator(90.f, 0.f, 0.f));
}
