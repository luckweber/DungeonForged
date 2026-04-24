// Source/DungeonForged/Private/FX/UDFCameraShakeFunctionLibrary.cpp
#include "FX/UDFCameraShakeFunctionLibrary.h"
#include "FX/UDFCameraShakes.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UDFCameraShakeFunctionLibrary::PlayLightHitOnOwner(
	const UObject* const WorldContextObject,
	APlayerController* const PC)
{
	if (IsRunningDedicatedServer() || !PC)
	{
		return;
	}
	if (APlayerCameraManager* const PCM = PC->PlayerCameraManager)
	{
		PCM->StartCameraShake(UDFCameraShake_LightHit::StaticClass());
	}
}

void UDFCameraShakeFunctionLibrary::PlayHeavyHitOnOwner(
	const UObject* const WorldContextObject,
	APlayerController* const PC)
{
	(void)WorldContextObject;
	if (IsRunningDedicatedServer() || !PC)
	{
		return;
	}
	if (APlayerCameraManager* const PCM = PC->PlayerCameraManager)
	{
		PCM->StartCameraShake(UDFCameraShake_HeavyHit::StaticClass());
	}
}

void UDFCameraShakeFunctionLibrary::PlayBossSlamOnOwner(
	const UObject* const WorldContextObject,
	APlayerController* const PC)
{
	(void)WorldContextObject;
	if (IsRunningDedicatedServer() || !PC)
	{
		return;
	}
	if (APlayerCameraManager* const PCM = PC->PlayerCameraManager)
	{
		PCM->StartCameraShake(UDFCameraShake_BossSlam::StaticClass());
	}
}

void UDFCameraShakeFunctionLibrary::PlayExplosionShakeAt(
	const UObject* const WorldContextObject,
	const FVector& Origin,
	const float InnerRadius,
	const float OuterRadius,
	const float Scale)
{
	if (IsRunningDedicatedServer() || !WorldContextObject)
	{
		return;
	}
	UGameplayStatics::PlayWorldCameraShake(
		WorldContextObject,
		UDFCameraShake_Explosion::StaticClass(),
		Origin,
		InnerRadius,
		OuterRadius,
		Scale,
		false);
}
