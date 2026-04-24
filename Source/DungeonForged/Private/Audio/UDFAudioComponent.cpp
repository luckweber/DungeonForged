// Source/DungeonForged/Private/Audio/UDFAudioComponent.cpp
#include "Audio/UDFAudioComponent.h"
#include "Audio/UDFSoundLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

UDFAudioComponent::UDFAudioComponent()
{
	bAutoActivate = false;
	PrimaryComponentTick.bCanEverTick = false;
}

bool UDFAudioComponent::ShouldRunAudio() const
{
	if (UWorld* const W = GetWorld())
	{
		return W->GetNetMode() != NM_DedicatedServer;
	}
	return true;
}

USoundBase* UDFAudioComponent::ResolveAbilitySound(const FGameplayTag& Tag) const
{
	if (const TObjectPtr<USoundBase>* A = AbilitySoundMap.Find(Tag))
	{
		if (A->Get())
		{
			return A->Get();
		}
	}
	if (FallbackTagLibrary)
	{
		if (USoundBase* S = FallbackTagLibrary->GetSoundForTag(Tag))
		{
			return S;
		}
	}
	return nullptr;
}

USoundBase* UDFAudioComponent::ResolveFootstep(const TEnumAsByte<EPhysicalSurface> Surface) const
{
	if (const TObjectPtr<USoundBase>* S = FootstepMap.Find(Surface))
	{
		if (S->Get())
		{
			return S->Get();
		}
	}
	// Fallback: try Default physical surface
	if (const TObjectPtr<USoundBase>* D = FootstepMap.Find(
			TEnumAsByte<EPhysicalSurface>(SurfaceType_Default)))
	{
		if (D->Get())
		{
			return D->Get();
		}
	}
	return nullptr;
}

USoundBase* UDFAudioComponent::ChooseImpact(const TEnumAsByte<EPhysicalSurface> Surface) const
{
	if (const FDFImpactLayerSound* I = ImpactBySurface.Find(Surface))
	{
		USoundBase* A = I->Sound0.Get();
		USoundBase* B = I->Sound1.Get();
		if (A && B)
		{
			return (FMath::RandBool()) ? A : B;
		}
		if (A)
		{
			return A;
		}
		if (B)
		{
			return B;
		}
	}
	// Reuse footstep as impact variant if not mapped
	return ResolveFootstep(Surface);
}

void UDFAudioComponent::PlayAbilitySound(
	const FGameplayTag& AbilityTag,
	const FVector& Location,
	const float Intensity)
{
	if (!ShouldRunAudio())
	{
		return;
	}
	USoundBase* const Snd = ResolveAbilitySound(AbilityTag);
	if (!Snd || !GetWorld())
	{
		return;
	}
	const float Vol = FMath::Clamp(Intensity, 0.1f, 1.f);
	UAudioComponent* const Sp = UGameplayStatics::SpawnSoundAtLocation(
		this,
		Snd,
		Location,
		FRotator::ZeroRotator,
		Vol,
		1.f,
		0.f,
		DefaultAttenuation3D,
		AbilitySFXConcurrency);
	if (Sp)
	{
		// If using UMetaSoundSource, optional parameter "Intensity" (see authoring notes in header)
		Sp->SetFloatParameter(FName("Intensity"), FMath::Clamp(Intensity, 0.f, 1.f));
	}
}

void UDFAudioComponent::PlayFootstep(
	const TEnumAsByte<EPhysicalSurface> Surface,
	const FVector& Location)
{
	if (!ShouldRunAudio())
	{
		return;
	}
	USoundBase* const Snd = ResolveFootstep(Surface);
	if (!Snd)
	{
		return;
	}
	UGameplayStatics::PlaySoundAtLocation(
		this,
		Snd,
		Location,
		FRotator::ZeroRotator,
		1.f,
		1.f,
		0.f,
		DefaultAttenuation3D,
		FootstepConcurrency);
}

void UDFAudioComponent::PlayImpactSound(
	const TEnumAsByte<EPhysicalSurface> Surface,
	const FVector& Location,
	const float ForceMagnitude)
{
	if (!ShouldRunAudio())
	{
		return;
	}
	USoundBase* const Snd = ChooseImpact(Surface);
	if (!Snd)
	{
		return;
	}
	const float Vol = FMath::GetMappedRangeValueClamped(
		FVector2D(0.f, 2000.f), FVector2D(0.2f, 1.f), ForceMagnitude);
	const float P = FMath::FRandRange(0.95f, 1.05f);
	UGameplayStatics::PlaySoundAtLocation(
		this,
		Snd,
		Location,
		FRotator::ZeroRotator,
		Vol,
		P,
		0.f,
		DefaultAttenuation3D,
		AbilitySFXConcurrency);
}

void UDFAudioComponent::PlayDFUISound(USoundBase* InSound, const float Volume)
{
	if (!ShouldRunAudio() || !InSound)
	{
		return;
	}
	UGameplayStatics::PlaySound2D(
		this,
		InSound,
		Volume,
		1.f,
		0.f,
		nullptr,
		GetOwner(),
		true);
}
