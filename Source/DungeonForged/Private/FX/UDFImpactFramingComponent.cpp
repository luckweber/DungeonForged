// Source/DungeonForged/Private/FX/UDFImpactFramingComponent.cpp
#include "FX/UDFImpactFramingComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HAL/PlatformTime.h"
#include "Localization/UDFAccessibilitySubsystem.h"
#include "TimerManager.h"

UDFImpactFramingComponent::UDFImpactFramingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UAnimInstance* UDFImpactFramingComponent::GetAnimInstance() const
{
	if (const ACharacter* const C = Cast<ACharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* const M = C->GetMesh())
		{
			return M->GetAnimInstance();
		}
	}
	return nullptr;
}

float UDFImpactFramingComponent::GetActiveFreezeRemainingSeconds() const
{
	if (!bRateActive)
	{
		return 0.f;
	}
	return static_cast<float>(FMath::Max(0.0, FreezeEndRealTime - FPlatformTime::Seconds()));
}

void UDFImpactFramingComponent::PlayBand(const EDFHitFeedbackBand Band, const float MagnitudeFactor)
{
	float Duration = LightFreezeDuration;
	switch (Band)
	{
	case EDFHitFeedbackBand::Heavy:
		Duration = HeavyFreezeDuration;
		break;
	case EDFHitFeedbackBand::Critical:
		Duration = CriticalFreezeDuration;
		break;
	case EDFHitFeedbackBand::Knockback:
		Duration = FMath::Max(CriticalFreezeDuration, HeavyFreezeDuration);
		break;
	default:
		break;
	}
	const float Factor = FMath::Clamp(MagnitudeFactor, 0.5f, 1.5f);
	float IntensityScale = 1.f;
	if (UWorld* const W = GetWorld())
	{
		if (UGameInstance* const GI = W->GetGameInstance())
		{
			if (const UDFAccessibilitySubsystem* const A11y = GI->GetSubsystem<UDFAccessibilitySubsystem>())
			{
				IntensityScale = A11y->GetHitStopIntensityScale();
			}
		}
	}
	if (IntensityScale <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	TriggerCustom(Duration * Factor * IntensityScale);
}

void UDFImpactFramingComponent::ApplyFreezeToMontage(
	UAnimMontage* const Montage, const float Duration, const float Rate)
{
	if (!Montage || Duration <= 0.f)
	{
		return;
	}
	UAnimInstance* const Anim = GetAnimInstance();
	if (!Anim || !Anim->Montage_IsPlaying(Montage))
	{
		return;
	}
	const float ClampedRate = FMath::Clamp(Rate, 0.01f, 1.f);
	const double Now = FPlatformTime::Seconds();
	const double NewEnd = Now + static_cast<double>(Duration);

	if (bRateActive && FrozenMontage.Get() == Montage)
	{
		FreezeEndRealTime = FMath::Max(FreezeEndRealTime, NewEnd);
		if (ClampedRate < ActiveFreezeRate)
		{
			ActiveFreezeRate = ClampedRate;
			Anim->Montage_SetPlayRate(Montage, ActiveFreezeRate);
		}
	}
	else
	{
		if (bRateActive)
		{
			RestoreRate();
		}
		PriorRate = Anim->Montage_GetPlayRate(Montage);
		if (PriorRate <= 0.f)
		{
			PriorRate = 1.f;
		}
		ActiveFreezeRate = ClampedRate;
		Anim->Montage_SetPlayRate(Montage, ActiveFreezeRate);
		FrozenMontage = Montage;
		bRateActive = true;
		FreezeEndRealTime = NewEnd;
	}

	if (UWorld* const W = GetWorld())
	{
		const float Remaining = GetActiveFreezeRemainingSeconds();
		W->GetTimerManager().ClearTimer(RestoreTimer);
		W->GetTimerManager().SetTimer(
			RestoreTimer,
			FTimerDelegate::CreateUObject(this, &UDFImpactFramingComponent::RestoreRate),
			FMath::Max(0.001f, Remaining),
			false);
	}
}

void UDFImpactFramingComponent::TriggerCustom(const float Duration)
{
	if (Duration <= 0.f)
	{
		return;
	}
	UAnimInstance* const Anim = GetAnimInstance();
	if (!Anim)
	{
		return;
	}
	UAnimMontage* const Active = Anim->GetCurrentActiveMontage();
	if (!Active)
	{
		return;
	}
	ApplyFreezeToMontage(Active, Duration, FreezeRate);
}

void UDFImpactFramingComponent::RestoreRate()
{
	if (!bRateActive)
	{
		return;
	}
	if (FPlatformTime::Seconds() < FreezeEndRealTime)
	{
		if (UWorld* const W = GetWorld())
		{
			const float Remaining = GetActiveFreezeRemainingSeconds();
			W->GetTimerManager().ClearTimer(RestoreTimer);
			W->GetTimerManager().SetTimer(
				RestoreTimer,
				FTimerDelegate::CreateUObject(this, &UDFImpactFramingComponent::RestoreRate),
				FMath::Max(0.001f, Remaining),
				false);
		}
		return;
	}
	bRateActive = false;
	FreezeEndRealTime = 0.0;
	ActiveFreezeRate = 1.f;
	UAnimInstance* const Anim = GetAnimInstance();
	UAnimMontage* const Montage = FrozenMontage.Get();
	if (Anim && Montage && Anim->Montage_IsPlaying(Montage))
	{
		Anim->Montage_SetPlayRate(Montage, PriorRate);
	}
	FrozenMontage.Reset();
}

void UDFImpactFramingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(RestoreTimer);
	}
	FreezeEndRealTime = 0.0;
	RestoreRate();
	Super::EndPlay(EndPlayReason);
}
