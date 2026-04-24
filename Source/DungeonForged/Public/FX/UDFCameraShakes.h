// Source/DungeonForged/Public/FX/UDFCameraShakes.h
#pragma once

#include "CoreMinimal.h"
#include "Shakes/LegacyCameraShake.h"
#include "UDFCameraShakes.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFCameraShake_LightHit : public ULegacyCameraShake
{
	GENERATED_BODY()
public:
	UDFCameraShake_LightHit();
};

UCLASS()
class DUNGEONFORGED_API UDFCameraShake_HeavyHit : public ULegacyCameraShake
{
	GENERATED_BODY()
public:
	UDFCameraShake_HeavyHit();
};

UCLASS()
class DUNGEONFORGED_API UDFCameraShake_BossSlam : public ULegacyCameraShake
{
	GENERATED_BODY()
public:
	UDFCameraShake_BossSlam();
};

UCLASS()
class DUNGEONFORGED_API UDFCameraShake_Explosion : public ULegacyCameraShake
{
	GENERATED_BODY()
public:
	UDFCameraShake_Explosion();
};
