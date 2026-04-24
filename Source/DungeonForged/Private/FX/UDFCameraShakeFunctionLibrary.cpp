// Source/DungeonForged/Private/FX/UDFCameraShakeFunctionLibrary.cpp
#include "FX/UDFCameraShakeFunctionLibrary.h"
#include "FX/UDFCameraShakes.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Localization/UDFAccessibilitySubsystem.h"

namespace
{
	float DF_ShakeScale(const UObject* WCO)
	{
		if (const UWorld* W = WCO ? WCO->GetWorld() : nullptr)
		{
			if (UGameInstance* const GI = W->GetGameInstance())
			{
				if (const UDFAccessibilitySubsystem* A11y = GI->GetSubsystem<UDFAccessibilitySubsystem>())
				{
					return A11y->GetCameraShakeAmplitudeScale();
				}
			}
		}
		return 1.f;
	}
}

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
		PCM->StartCameraShake(UDFCameraShake_LightHit::StaticClass(), DF_ShakeScale(WorldContextObject));
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
		PCM->StartCameraShake(UDFCameraShake_HeavyHit::StaticClass(), DF_ShakeScale(WorldContextObject));
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
		PCM->StartCameraShake(UDFCameraShake_BossSlam::StaticClass(), DF_ShakeScale(WorldContextObject));
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
		Scale * DF_ShakeScale(WorldContextObject),
		false);
}
