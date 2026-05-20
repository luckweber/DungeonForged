// Source/DungeonForged/Private/FX/UDFHitStopSubsystem.cpp
#include "FX/UDFHitStopSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "FX/UDFCombatFeedbackTypes.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Localization/UDFAccessibilitySubsystem.h"
#include "DungeonForgedModule.h"

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

void UDFHitStopSubsystem::PlayBand(
	const EDFHitFeedbackBand Band,
	AActor* const ExcludeActor,
	const float MagnitudeFactor)
{
	float Duration = 0.06f;
	float Dilation = 0.05f;
	switch (Band)
	{
	case EDFHitFeedbackBand::Heavy:
		Duration = 0.10f;
		Dilation = 0.02f;
		break;
	case EDFHitFeedbackBand::Critical:
		Duration = 0.14f;
		Dilation = 0.01f;
		break;
	case EDFHitFeedbackBand::Knockback:
		Duration = 0.20f;
		Dilation = 0.0f;
		break;
	default:
		break;
	}
	const float Factor = FMath::Clamp(MagnitudeFactor, 0.5f, 1.5f);
	TriggerHitStop(Duration * Factor, Dilation, ExcludeActor);
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
	float IntensityScale = 1.f;
	if (UGameInstance* const GI = W->GetGameInstance())
	{
		if (const UDFAccessibilitySubsystem* const A11y = GI->GetSubsystem<UDFAccessibilitySubsystem>())
		{
			IntensityScale = A11y->GetHitStopIntensityScale();
		}
	}
	if (IntensityScale <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	const float ScaledDuration = Duration * IntensityScale;
	const float ScaledDilation = FMath::Lerp(1.f, TimeDilation, IntensityScale);
	const double Now = FPlatformTime::Seconds();
	const double Remaining = bInHitStop ? FMath::Max(0.0, static_cast<double>(HitStopEndRealTime) - Now) : 0.0;
	if (bInHitStop && ScaledDuration <= Remaining)
	{
		return;
	}
	if (bInHitStop)
	{
		EndHitStop();
	}
	const float D = FMath::Max(0.0001f, ScaledDuration);
	const float G = SafeGlobalDilation(ScaledDilation);
	ApplyHitStop(G, ExcludeActor);
	HitStopEndRealTime = Now + static_cast<double>(D);
	UE_LOG(LogDFTuning, Verbose, TEXT("HitStop dur=%.3f dil=%.3f scale=%.2f instigator=%s"),
		D, G, IntensityScale, *GetNameSafe(ExcludeActor));
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

float UDFHitStopSubsystem::GetHitStopRemainingSeconds() const
{
	if (!bInHitStop)
	{
		return 0.f;
	}
	return static_cast<float>(FMath::Max(0.0, HitStopEndRealTime - FPlatformTime::Seconds()));
}
