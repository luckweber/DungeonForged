// Source/DungeonForged/Private/FX/UDFHitStopSubsystem.cpp
#include "FX/UDFHitStopSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformTime.h"

float UDFHitStopSubsystem::SafeGlobalDilation(const float Requested)
{
	if (Requested <= 0.f)
	{
		return MinGlobalDilation;
	}
	return FMath::Clamp(Requested, MinGlobalDilation, 1.f);
}

TStatId UDFHitStopSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UDFHitStopSubsystem, STATGROUP_Tickables);
}

void UDFHitStopSubsystem::Tick(float DeltaTime)
{
	(void)DeltaTime;
	if (!bInHitStop)
	{
		return;
	}
	if (FPlatformTime::Seconds() < HitStopEndRealTime)
	{
		return;
	}
	EndHitStop();
}

void UDFHitStopSubsystem::Deinitialize()
{
	EndHitStop();
	Super::Deinitialize();
}

void UDFHitStopSubsystem::SetExcludedActorDilation(AActor* const ExcludeActor, const float GlobalDilation)
{
	if (!IsValid(ExcludeActor))
	{
		return;
	}
	const float G = (GlobalDilation > KINDA_SMALL_NUMBER) ? GlobalDilation : MinGlobalDilation;
	ExcludeActor->CustomTimeDilation = 1.f / G;
}

void UDFHitStopSubsystem::TriggerHitStop(const float Duration, const float TimeDilation, AActor* const ExcludeActor)
{
	UWorld* const W = GetWorld();
	if (!W || W->bIsTearingDown)
	{
		return;
	}
	const double Now = FPlatformTime::Seconds();
	const double Remaining = bInHitStop ? FMath::Max(0.0, static_cast<double>(HitStopEndRealTime) - Now) : 0.0;
	if (bInHitStop && Duration <= Remaining)
	{
		return;
	}
	if (bInHitStop)
	{
		EndHitStop();
	}
	const float D = FMath::Max(0.0001f, Duration);
	const float G = SafeGlobalDilation(TimeDilation);
	ApplyHitStop(G, ExcludeActor);
	HitStopEndRealTime = Now + static_cast<double>(D);
}

void UDFHitStopSubsystem::ApplyHitStop(const float TimeDilation, AActor* const ExcludeActor)
{
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	bInHitStop = true;
	UGameplayStatics::SetGlobalTimeDilation(W, TimeDilation);
	ExcludedActor = ExcludeActor;
	SetExcludedActorDilation(ExcludeActor, TimeDilation);
}

void UDFHitStopSubsystem::EndHitStop()
{
	UWorld* const W = GetWorld();
	if (ExcludedActor.IsValid())
	{
		ExcludedActor->CustomTimeDilation = 1.f;
		ExcludedActor = nullptr;
	}
	if (W && !W->bIsTearingDown)
	{
		UGameplayStatics::SetGlobalTimeDilation(W, 1.f);
	}
	bInHitStop = false;
}
