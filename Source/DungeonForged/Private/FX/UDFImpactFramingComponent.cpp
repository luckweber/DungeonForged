// Source/DungeonForged/Private/FX/UDFImpactFramingComponent.cpp
#include "FX/UDFImpactFramingComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
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

void UDFImpactFramingComponent::TriggerCustom(const float Duration)
{
	if (Duration <= 0.f || bRateActive)
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
	PriorRate = Anim->Montage_GetPlayRate(Active);
	if (PriorRate <= 0.f)
	{
		PriorRate = 1.f;
	}
	Anim->Montage_SetPlayRate(Active, FreezeRate);
	FrozenMontage = Active;
	bRateActive = true;

	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			RestoreTimer,
			FTimerDelegate::CreateUObject(this, &UDFImpactFramingComponent::RestoreRate),
			Duration,
			false);
	}
}

void UDFImpactFramingComponent::RestoreRate()
{
	if (!bRateActive)
	{
		return;
	}
	bRateActive = false;
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
	RestoreRate();
	Super::EndPlay(EndPlayReason);
}
