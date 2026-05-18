// Source/DungeonForged/Private/FX/UDFCombatFeedbackLibrary.cpp
#include "FX/UDFCombatFeedbackLibrary.h"
#include "Engine/World.h"
#include "FX/UDFCameraShakeFunctionLibrary.h"
#include "FX/UDFHitStopSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UDFCombatFeedbackLibrary::PlayHitFeedbackForBand(
	UObject* const WorldContextObject,
	const EDFHitFeedbackBand Band,
	AActor* const InstigatorActor)
{
	UWorld* const World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (UDFHitStopSubsystem* const HS = World->GetSubsystem<UDFHitStopSubsystem>())
	{
		switch (Band)
		{
		case EDFHitFeedbackBand::Light: HS->LightHit(InstigatorActor); break;
		case EDFHitFeedbackBand::Heavy: HS->HeavyHit(InstigatorActor); break;
		case EDFHitFeedbackBand::Critical: HS->CriticalHit(InstigatorActor); break;
		case EDFHitFeedbackBand::Knockback: HS->BossSlam(InstigatorActor); break;
		default: break;
		}
	}
	if (APlayerController* const PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		switch (Band)
		{
		case EDFHitFeedbackBand::Light: UDFCameraShakeFunctionLibrary::PlayLightHitOnOwner(WorldContextObject, PC); break;
		case EDFHitFeedbackBand::Heavy: UDFCameraShakeFunctionLibrary::PlayHeavyHitOnOwner(WorldContextObject, PC); break;
		case EDFHitFeedbackBand::Critical: UDFCameraShakeFunctionLibrary::PlayHeavyHitOnOwner(WorldContextObject, PC); break;
		case EDFHitFeedbackBand::Knockback: UDFCameraShakeFunctionLibrary::PlayBossSlamOnOwner(WorldContextObject, PC); break;
		default: break;
		}
	}
}

void UDFCombatFeedbackLibrary::PlayHitFeedbackFromDamage(
	UObject* const WorldContextObject,
	const float DamageMagnitude,
	const float MaxHealth,
	const bool bIsKnockback,
	AActor* const InstigatorActor)
{
	EDFHitFeedbackBand Band = EDFHitFeedbackBand::Light;
	if (bIsKnockback)
	{
		Band = EDFHitFeedbackBand::Knockback;
	}
	else
	{
		const float Pct = MaxHealth > KINDA_SMALL_NUMBER ? (DamageMagnitude / MaxHealth) : 0.f;
		if (Pct > 0.3f)
		{
			Band = EDFHitFeedbackBand::Critical;
		}
		else if (DamageMagnitude >= 25.f)
		{
			Band = EDFHitFeedbackBand::Heavy;
		}
	}
	PlayHitFeedbackForBand(WorldContextObject, Band, InstigatorActor);
}
