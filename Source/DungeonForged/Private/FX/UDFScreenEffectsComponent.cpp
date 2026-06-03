// Source/DungeonForged/Private/FX/UDFScreenEffectsComponent.cpp
#include "FX/UDFScreenEffectsComponent.h"
#include "FX/UDFCameraShakeFunctionLibrary.h"
#include "FX/UDFHitStopSubsystem.h"
#include "FX/UDFImpactFramingComponent.h"
#include "Characters/ADFPlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "GAS/DFGameplayTags.h"
#include "Camera/CameraComponent.h"
#include "Components/PostProcessComponent.h"
#include "GAS/UDFAttributeSet.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SceneComponent.h"
#include "Localization/UDFAccessibilitySubsystem.h"
#include "Math/UnrealMathUtility.h"

namespace
{
	constexpr float kLowHealthThresh = 0.25f;
	constexpr float kLowVigMin = 0.3f;
	constexpr float kLowVigMax = 0.7f;
	constexpr float kLowPulseSec = 1.5f;

	float DF_VfxScaleFromWorld(UWorld* const W)
	{
		if (!W)
		{
			return 1.f;
		}
		if (UGameInstance* const GI = W->GetGameInstance())
		{
			if (const UDFAccessibilitySubsystem* A11y = GI->GetSubsystem<UDFAccessibilitySubsystem>())
			{
				return A11y->GetVFXIntensityScale();
			}
		}
		return 1.f;
	}
}

UDFScreenEffectsComponent::UDFScreenEffectsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UDFScreenEffectsComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UDFAttributeSet* const S = BoundAttributeSet.Get())
	{
		S->OnHealthChanged.RemoveAll(this);
		S->OnOutOfHealth.RemoveAll(this);
	}
	BoundAttributeSet = nullptr;
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearAllTimersForObject(this);
	}
	if (bDeathInProgress && GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
	}
	Super::EndPlay(Reason);
}

void UDFScreenEffectsComponent::OnGASReady(UDFAttributeSet* const InAttributeSet)
{
	if (!InAttributeSet)
	{
		if (UDFAttributeSet* const S = BoundAttributeSet.Get())
		{
			S->OnHealthChanged.RemoveAll(this);
			S->OnOutOfHealth.RemoveAll(this);
		}
		BoundAttributeSet = nullptr;
		return;
	}
	if (BoundAttributeSet.Get() == InAttributeSet)
	{
		return;
	}
	if (UDFAttributeSet* const Old = BoundAttributeSet.Get())
	{
		Old->OnHealthChanged.RemoveAll(this);
		Old->OnOutOfHealth.RemoveAll(this);
	}
	BoundAttributeSet = InAttributeSet;
	InAttributeSet->OnHealthChanged.AddUObject(this, &UDFScreenEffectsComponent::HandleHealthChanged);
	InAttributeSet->OnOutOfHealth.AddUObject(this, &UDFScreenEffectsComponent::HandleOutOfHealth);
	HandleHealthChanged(InAttributeSet->GetHealth(), InAttributeSet->GetMaxHealth());
}

void UDFScreenEffectsComponent::HandleHealthChanged(const float H, const float M)
{
	LowHealthSetEnabledFromRatio(H, M);
}

void UDFScreenEffectsComponent::HandleOutOfHealth()
{
	OnDeath();
}

void UDFScreenEffectsComponent::LowHealthSetEnabledFromRatio(const float H, const float M)
{
	if (M <= KINDA_SMALL_NUMBER)
	{
		bLowHealthVignette = false;
		return;
	}
	bLowHealthVignette = !bBerserk && ((H / M) < kLowHealthThresh);
}

void UDFScreenEffectsComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* const O = GetOwner();
	if (O && !PostProcessComp)
	{
		PostProcessComp = NewObject<UPostProcessComponent>(O, UPostProcessComponent::StaticClass(), TEXT("DF_ScreenPP"));
		PostProcessComp->bUnbound = true;
		PostProcessComp->bEnabled = true;
		PostProcessComp->BlendWeight = 1.f;
		PostProcessComp->Priority = 100.f;
		if (ACharacter* const C = Cast<ACharacter>(O))
		{
			if (USceneComponent* const R = C->GetRootComponent())
			{
				PostProcessComp->SetupAttachment(R);
			}
		}
		O->AddInstanceComponent(PostProcessComp);
		PostProcessComp->RegisterComponent();
	}
	if (ScreenEffectParentMaterial)
	{
		ScreenEffectMaterial = UMaterialInstanceDynamic::Create(ScreenEffectParentMaterial, this);
	}
	if (PostProcessComp)
	{
		if (ScreenEffectMaterial)
		{
			PostProcessComp->AddOrUpdateBlendable(ScreenEffectMaterial, 1.f);
		}
	}
	if (ADFPlayerCharacter* const Ch = Cast<ADFPlayerCharacter>(GetOwner()))
	{
		if (Ch->FollowCamera)
		{
			BaseFOV = Ch->FollowCamera->FieldOfView;
		}
	}
}

void UDFScreenEffectsComponent::SetVignette(const float Intensity, const FLinearColor& Tint, const bool bFromLowHealth)
{
	(void)bFromLowHealth;
	if (ScreenEffectMaterial)
	{
		ScreenEffectMaterial->SetScalarParameterValue(FName("VignetteIntensity"), FMath::Clamp(Intensity, 0.f, 1.f));
		ScreenEffectMaterial->SetVectorParameterValue(
			FName("VignetteColor"), FVector4(Tint.R, Tint.G, Tint.B, Tint.A));
		return;
	}
	if (PostProcessComp)
	{
		FPostProcessSettings& S = PostProcessComp->Settings;
		S.bOverride_VignetteIntensity = true;
		S.VignetteIntensity = FMath::Clamp(Intensity, 0.f, 1.f);
		PostProcessComp->bEnabled = true;
	}
}

void UDFScreenEffectsComponent::SetChroma(const float Intensity)
{
	if (ScreenEffectMaterial)
	{
		ScreenEffectMaterial->SetScalarParameterValue(FName("ChromaticAberration"), FMath::Max(0.f, Intensity));
		return;
	}
	if (PostProcessComp)
	{
		// UE 5.4: chromatic aberration intensity is SceneFringeIntensity (not a top-level ChromaticAberration on FPostProcessSettings).
		PostProcessComp->Settings.bOverride_SceneFringeIntensity = true;
		PostProcessComp->Settings.SceneFringeIntensity = FMath::Clamp(Intensity, 0.f, 5.f);
	}
}

void UDFScreenEffectsComponent::SetBlurAmount(const float Amt)
{
	const float Clamped = FMath::Clamp(Amt, 0.f, 1.f);
	if (ScreenEffectMaterial)
	{
		ScreenEffectMaterial->SetScalarParameterValue(FName("BlurAmount"), Clamped);
		return;
	}
	if (PostProcessComp)
	{
		PostProcessComp->Settings.bOverride_MotionBlurAmount = true;
		PostProcessComp->Settings.MotionBlurAmount = Clamped;
		PostProcessComp->bEnabled = true;
	}
}

void UDFScreenEffectsComponent::BlurPulse(const float Duration, const float Intensity)
{
	BlurInit = FMath::Clamp(Intensity, 0.f, 1.f);
	BlurInitDuration = FMath::Max(0.01f, Duration);
	BlurTimeRemaining = BlurInitDuration;
	SetBlurAmount(BlurInit);
}

void UDFScreenEffectsComponent::SetHitVignette(const float Intensity, const FLinearColor& Tint, const float Duration)
{
	HitVignetteIntensity = FMath::Clamp(Intensity, 0.f, 1.f);
	HitVignetteTint = Tint;
	HitVignetteDuration = FMath::Max(0.01f, Duration);
	HitVignetteTimeRemaining = HitVignetteDuration;
}

void UDFScreenEffectsComponent::SetSaturationMult(const float Mult)
{
	if (ScreenEffectMaterial)
	{
		ScreenEffectMaterial->SetScalarParameterValue(FName("SaturationMult"), Mult);
		return;
	}
	if (PostProcessComp)
	{
		PostProcessComp->Settings.bOverride_ColorSaturation = true;
		const float U = 0.5f * (Mult + 1.f);
		PostProcessComp->Settings.ColorSaturation = FVector4(U, U, U, 1.f);
	}
}

void UDFScreenEffectsComponent::SetFilmGrain(const float Intensity)
{
	if (ScreenEffectMaterial)
	{
		ScreenEffectMaterial->SetScalarParameterValue(FName("GrainAmount"), FMath::Clamp(Intensity, 0.f, 1.f));
		return;
	}
	if (PostProcessComp)
	{
		PostProcessComp->Settings.bOverride_FilmGrainIntensity = true;
		PostProcessComp->Settings.FilmGrainIntensity = FMath::Clamp(Intensity, 0.f, 1.f);
	}
}

void UDFScreenEffectsComponent::SetFlash(const float I, const FLinearColor& C)
{
	CurrentFlash = I;
	CurrentFlashColor = C;
	if (ScreenEffectMaterial)
	{
		ScreenEffectMaterial->SetScalarParameterValue(FName("FlashIntensity"), I);
		ScreenEffectMaterial->SetVectorParameterValue(
			FName("FlashColor"), FVector4(C.R, C.G, C.B, C.A));
	}
}

void UDFScreenEffectsComponent::LerpLocalPlayerFOV(
	const float TargetFOV, const float DeltaTime, const float InterpSpeed)
{
	ADFPlayerCharacter* const Ch = Cast<ADFPlayerCharacter>(GetOwner());
	if (!Ch || !Ch->FollowCamera)
	{
		return;
	}
	Ch->FollowCamera->SetFieldOfView(
		FMath::FInterpTo(Ch->FollowCamera->FieldOfView, TargetFOV, DeltaTime, InterpSpeed));
}

void UDFScreenEffectsComponent::ResolveAndApplyVignette()
{
	struct FCandidate
	{
		EDFScreenVignettePriority Priority = EDFScreenVignettePriority::None;
		float Intensity = 0.f;
		FLinearColor Tint = FLinearColor::White;
	};
	FCandidate Best;
	auto Consider = [&Best](const EDFScreenVignettePriority Priority, const float Intensity, const FLinearColor& Tint)
	{
		if (Intensity <= KINDA_SMALL_NUMBER)
		{
			return;
		}
		if (static_cast<uint8>(Priority) >= static_cast<uint8>(Best.Priority))
		{
			Best.Priority = Priority;
			Best.Intensity = Intensity;
			Best.Tint = Tint;
		}
	};

	if (bDeathInProgress)
	{
		const float D = FMath::Clamp(DeathFXTime / 2.f, 0.f, 1.f);
		Consider(EDFScreenVignettePriority::Death, 0.2f + 0.6f * D, FLinearColor::Black);
	}
	if (bBerserk)
	{
		Consider(EDFScreenVignettePriority::Berserk, 0.5f, FLinearColor::Red);
	}
	if (HitVignetteTimeRemaining > 0.f)
	{
		Consider(EDFScreenVignettePriority::Hit, HitVignetteIntensity, HitVignetteTint);
	}
	if (bLowHealthVignette && !bBerserk && !bDeathInProgress)
	{
		const float P = FMath::Fmod(LowHealthPhase, kLowPulseSec) / kLowPulseSec;
		const float W = 0.5f + 0.5f * FMath::Sin(P * 2.f * PI);
		const float Vi = FMath::Lerp(kLowVigMin, kLowVigMax, W);
		Consider(EDFScreenVignettePriority::LowHealth, Vi, FLinearColor(0.7f, 0.1f, 0.1f, 1.f));
	}

	SetVignette(Best.Intensity, Best.Tint, Best.Priority == EDFScreenVignettePriority::LowHealth);
}

void UDFScreenEffectsComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (IsRunningDedicatedServer())
	{
		return;
	}
	const APawn* const OwnerPawn = GetOwner() ? Cast<APawn>(GetOwner()) : nullptr;
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
	{
		return;
	}
	if (FlashTimeRemaining > 0.f)
	{
		FlashTimeRemaining = FMath::Max(0.f, FlashTimeRemaining - DeltaTime);
		const float T = 1.f - (FlashTimeRemaining / FMath::Max(0.0001f, FlashDuration));
		const float I = (1.f - T) * CurrentFlash;
		SetFlash(I, CurrentFlashColor);
	}
	if (ChromaTimeRemaining > 0.f)
	{
		ChromaTimeRemaining = FMath::Max(0.f, ChromaTimeRemaining - DeltaTime);
		if (ChromaTimeRemaining <= 0.f)
		{
			ChromaInit = 0.f;
			SetChroma(0.f);
		}
		else
		{
			const float Alpha = (ChromaInitDuration > KINDA_SMALL_NUMBER)
				? (ChromaTimeRemaining / ChromaInitDuration)
				: 0.f;
			SetChroma(ChromaInit * Alpha);
		}
	}
	if (BlurTimeRemaining > 0.f && !bDeathInProgress && !bBerserk)
	{
		BlurTimeRemaining = FMath::Max(0.f, BlurTimeRemaining - DeltaTime);
		if (BlurTimeRemaining <= 0.f)
		{
			BlurInit = 0.f;
			SetBlurAmount(0.f);
		}
		else
		{
			const float Alpha = (BlurInitDuration > KINDA_SMALL_NUMBER)
				? (BlurTimeRemaining / BlurInitDuration)
				: 0.f;
			SetBlurAmount(BlurInit * Alpha);
		}
	}
	if (bBerserk)
	{
		if (FDFGameplayTags::State_Berserk.IsValid())
		{
			if (const APawn* const P = Cast<APawn>(GetOwner()))
			{
				if (const UAbilitySystemComponent* const ASC = P->FindComponentByClass<UAbilitySystemComponent>())
				{
					if (!ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Berserk))
					{
						BerserkSetActive(false);
					}
				}
			}
		}
		if (bBerserk)
		{
			SetFilmGrain(0.12f);
			if (!bDeathInProgress)
			{
				SetBlurAmount(0.08f);
			}
			LerpLocalPlayerFOV(100.f, DeltaTime, 2.f);
		}
	}
	if (bLowHealthVignette && !bBerserk)
	{
		LowHealthPhase += DeltaTime;
	}
	if (bHealEffectActive)
	{
		HealEffectElapsed += DeltaTime;
		const float t = FMath::Clamp(HealEffectElapsed / HealEffectDuration, 0.f, 1.f);
		if (t < 0.15f)
		{
			const float s = 1.f - t / 0.15f;
			SetSaturationMult(0.2f * s + 0.4f);
		}
		else if (t < 0.3f)
		{
			const float s = (t - 0.15f) / 0.15f;
			SetSaturationMult(FMath::Lerp(0.4f, 1.4f, s));
		}
		else
		{
			const float s = (t - 0.3f) / 0.7f;
			SetSaturationMult(FMath::Lerp(1.4f, 1.f, s));
		}
		if (t >= 1.f)
		{
			bHealEffectActive = false;
			SetSaturationMult(1.f);
		}
	}
	if (bDodgeFOVActive)
	{
		DodgeFOVElapsed += DeltaTime;
		const float Alpha = DodgeFOVDuration > KINDA_SMALL_NUMBER
			? FMath::Clamp(DodgeFOVElapsed / DodgeFOVDuration, 0.f, 1.f)
			: 1.f;
		LerpLocalPlayerFOV(BaseFOV, DeltaTime, 8.f);
		if (Alpha >= 1.f)
		{
			bDodgeFOVActive = false;
			if (ADFPlayerCharacter* const Ch = Cast<ADFPlayerCharacter>(GetOwner()))
			{
				if (Ch->FollowCamera)
				{
					Ch->FollowCamera->SetFieldOfView(BaseFOV);
				}
			}
		}
	}
	if (HitVignetteTimeRemaining > 0.f)
	{
		HitVignetteTimeRemaining = FMath::Max(0.f, HitVignetteTimeRemaining - DeltaTime);
	}
	if (SpectacleBloomTimeRemaining > 0.f)
	{
		SpectacleBloomTimeRemaining = FMath::Max(0.f, SpectacleBloomTimeRemaining - DeltaTime);
		if (PostProcessComp)
		{
			const float Alpha = SpectacleBloomTimeRemaining / FMath::Max(0.0001f, 0.55f);
			PostProcessComp->Settings.BloomIntensity = 1.f + SpectacleBloomBoost * Alpha;
			if (SpectacleBloomTimeRemaining <= 0.f)
			{
				PostProcessComp->Settings.bOverride_BloomIntensity = false;
				SpectacleBloomBoost = 0.f;
			}
		}
	}
	if (bSpectacleFOVActive)
	{
		SpectacleFOVElapsed += DeltaTime;
		const float Alpha = SpectacleFOVDuration > KINDA_SMALL_NUMBER
			? FMath::Clamp(SpectacleFOVElapsed / SpectacleFOVDuration, 0.f, 1.f)
			: 1.f;
		if (ADFPlayerCharacter* const Ch = Cast<ADFPlayerCharacter>(GetOwner()))
		{
			if (Ch->FollowCamera)
			{
				const float Target = Alpha < 0.5f
					? FMath::Lerp(BaseFOV, SpectacleFOVTarget, Alpha * 2.f)
					: FMath::Lerp(SpectacleFOVTarget, BaseFOV, (Alpha - 0.5f) * 2.f);
				Ch->FollowCamera->SetFieldOfView(Target);
			}
		}
		if (Alpha >= 1.f)
		{
			bSpectacleFOVActive = false;
		}
	}
	if (bDeathInProgress)
	{
		DeathFXTime += DeltaTime;
		const float D = FMath::Clamp(DeathFXTime / 2.f, 0.f, 1.f);
		SetSaturationMult(1.f - D);
		SetBlurAmount(D * 0.65f);
	}
	ResolveAndApplyVignette();
}

void UDFScreenEffectsComponent::DamageReceived(const float DamagePercent)
{
	const float S = DF_VfxScaleFromWorld(GetWorld());
	FlashScreen(FLinearColor::Red, 0.15f, DamagePercent * 0.8f * S);
	if (DamagePercent > 0.3f)
	{
		if (const APawn* const P = Cast<APawn>(GetOwner()))
		{
			if (APlayerController* const PC = Cast<APlayerController>(P->GetController()))
			{
				UDFCameraShakeFunctionLibrary::PlayHeavyHitOnOwner(this, PC);
			}
		}
		BlurPulse(0.12f, FMath::Clamp(DamagePercent * 0.35f, 0.08f, 0.35f) * S);
	}
	ChromaticAberrationPulse(0.3f, FMath::Min(1.f, DamagePercent * 2.f) * S);
}

void UDFScreenEffectsComponent::RefreshAccessibilityPresentation()
{
	if (UDFAttributeSet* const Attr = BoundAttributeSet.Get())
	{
		HandleHealthChanged(Attr->GetHealth(), Attr->GetMaxHealth());
	}
}

void UDFScreenEffectsComponent::HealReceived(const float HealPercent)
{
	FlashScreen(FLinearColor::Green, 0.2f, HealPercent * 0.5f);
	bHealEffectActive = true;
	HealEffectElapsed = 0.f;
	HealEffectDuration = 0.5f;
}

void UDFScreenEffectsComponent::BerserkSetActive(const bool bActive)
{
	bBerserk = bActive;
	if (!bBerserk)
	{
		SetFilmGrain(0.f);
		SetBlurAmount(0.f);
		BlurTimeRemaining = 0.f;
		if (ADFPlayerCharacter* const Ch = Cast<ADFPlayerCharacter>(GetOwner()))
		{
			if (Ch->FollowCamera)
			{
				Ch->FollowCamera->SetFieldOfView(BaseFOV);
			}
		}
	}
}

void UDFScreenEffectsComponent::OnDeath()
{
	if (UWorld* const W = GetWorld())
	{
		if (UDFHitStopSubsystem* const HS = W->GetSubsystem<UDFHitStopSubsystem>())
		{
			HS->TriggerHitStop(3.f, 0.2f, nullptr);
		}
	}
	bDeathInProgress = true;
	DeathFXTime = 0.f;
}

void UDFScreenEffectsComponent::TeleportOrBlink()
{
	const float S = DF_VfxScaleFromWorld(GetWorld());
	FlashScreen(FLinearColor::White, 0.05f, 0.6f * S);
	ChromaticAberrationPulse(0.2f, 0.5f * S);
}

void UDFScreenEffectsComponent::ApplyDodgeJuice(const float Duration)
{
	const float S = DF_VfxScaleFromWorld(GetWorld());
	const float D = FMath::Max(0.05f, Duration);
	FlashScreen(FLinearColor(0.35f, 0.55f, 1.f, 1.f), 0.08f, 0.45f * S);
	ChromaticAberrationPulse(D, 0.6f * S);
	BlurPulse(D, 0.18f * S);
	if (ADFPlayerCharacter* const Ch = Cast<ADFPlayerCharacter>(GetOwner()))
	{
		if (APlayerController* const PC = Ch->GetController<APlayerController>())
		{
			if (PC->IsLocalController())
			{
				UDFCameraShakeFunctionLibrary::PlayLightHitOnOwner(this, PC);
			}
		}
		if (Ch->FollowCamera)
		{
			BaseFOV = Ch->FollowCamera->FieldOfView;
			Ch->FollowCamera->SetFieldOfView(FMath::Min(BaseFOV + 6.f, 110.f));
			bDodgeFOVActive = true;
			DodgeFOVElapsed = 0.f;
			DodgeFOVDuration = D;
		}
	}
}

void UDFScreenEffectsComponent::ApplyKillSpectacle()
{
	const float S = DF_VfxScaleFromWorld(GetWorld());
	FlashScreen(FLinearColor(1.f, 0.85f, 0.4f, 1.f), 0.12f, 0.55f * S);
	ChromaticAberrationPulse(0.25f, 0.75f * S);
	BlurPulse(0.2f, 0.22f * S);
	SetSaturationMult(1.15f);
	SpectacleBloomBoost = 0.8f * S;
	SpectacleBloomTimeRemaining = 0.35f;
	if (ADFPlayerCharacter* const Ch = Cast<ADFPlayerCharacter>(GetOwner()))
	{
		if (Ch->FollowCamera)
		{
			BaseFOV = Ch->FollowCamera->FieldOfView;
			SpectacleFOVTarget = FMath::Min(BaseFOV + 8.f, 112.f);
			bSpectacleFOVActive = true;
			SpectacleFOVElapsed = 0.f;
			SpectacleFOVDuration = 0.45f;
		}
	}
	if (PostProcessComp)
	{
		PostProcessComp->Settings.bOverride_BloomIntensity = true;
		PostProcessComp->Settings.BloomIntensity = 1.f + SpectacleBloomBoost;
	}
}

void UDFScreenEffectsComponent::ApplyRoomClearSpectacle()
{
	const float S = DF_VfxScaleFromWorld(GetWorld());
	FlashScreen(FLinearColor(0.9f, 0.95f, 1.f, 1.f), 0.2f, 0.35f * S);
	ChromaticAberrationPulse(0.4f, 0.5f * S);
	SetHitVignette(0.35f, FLinearColor(0.05f, 0.1f, 0.2f, 1.f), 0.5f);
	SpectacleBloomBoost = 0.5f * S;
	SpectacleBloomTimeRemaining = 0.55f;
	if (ADFPlayerCharacter* const Ch = Cast<ADFPlayerCharacter>(GetOwner()))
	{
		if (Ch->FollowCamera)
		{
			BaseFOV = Ch->FollowCamera->FieldOfView;
			SpectacleFOVTarget = FMath::Min(BaseFOV + 5.f, 108.f);
			bSpectacleFOVActive = true;
			SpectacleFOVElapsed = 0.f;
			SpectacleFOVDuration = 0.55f;
		}
	}
	if (PostProcessComp)
	{
		PostProcessComp->Settings.bOverride_BloomIntensity = true;
		PostProcessComp->Settings.BloomIntensity = 1.f + SpectacleBloomBoost;
	}
}

void UDFScreenEffectsComponent::ApplyHitFromCombat(
	const EDFHitFeedbackBand Band,
	const float DamagePercent,
	AActor* const /*InstigatorActor*/,
	APlayerController* const PC)
{
	const float S = DF_VfxScaleFromWorld(GetWorld());
	float HitStopSync = 0.15f;
	if (const ADFPlayerCharacter* const OwnerPlayer = Cast<ADFPlayerCharacter>(GetOwner()))
	{
		if (OwnerPlayer->ImpactFraming)
		{
			HitStopSync = FMath::Max(HitStopSync, OwnerPlayer->ImpactFraming->GetActiveFreezeRemainingSeconds());
		}
	}
	if (UWorld* const W = GetWorld())
	{
		if (const UDFHitStopSubsystem* const HS = W->GetSubsystem<UDFHitStopSubsystem>())
		{
			HitStopSync = FMath::Max(HitStopSync, HS->GetHitStopRemainingSeconds());
		}
	}
	switch (Band)
	{
	case EDFHitFeedbackBand::Light:
		UDFCameraShakeFunctionLibrary::PlayLightHitOnOwner(this, PC);
		SetHitVignette(
			FMath::Clamp(DamagePercent * 0.35f, 0.08f, 0.35f),
			FLinearColor(0.5f, 0.f, 0.f, 1.f),
			HitStopSync);
		break;
	case EDFHitFeedbackBand::Heavy:
		DamageReceived(DamagePercent);
		UDFCameraShakeFunctionLibrary::PlayHeavyHitOnOwner(this, PC);
		SetHitVignette(
			FMath::Clamp(DamagePercent * 0.55f, 0.15f, 0.55f),
			FLinearColor(0.65f, 0.f, 0.f, 1.f),
			HitStopSync);
		break;
	case EDFHitFeedbackBand::Critical:
		DamageReceived(DamagePercent);
		ChromaticAberrationPulse(HitStopSync, FMath::Min(1.f, DamagePercent * 2.f) * S);
		BlurPulse(HitStopSync, FMath::Clamp(DamagePercent * 0.4f, 0.12f, 0.45f));
		UDFCameraShakeFunctionLibrary::PlayHeavyHitOnOwner(this, PC);
		SetHitVignette(
			FMath::Clamp(DamagePercent * 0.65f, 0.2f, 0.65f),
			FLinearColor(0.85f, 0.1f, 0.f, 1.f),
			HitStopSync);
		break;
	case EDFHitFeedbackBand::Knockback:
		DamageReceived(DamagePercent);
		ChromaticAberrationPulse(HitStopSync, FMath::Min(2.f, DamagePercent * 2.5f) * S);
		BlurPulse(HitStopSync * 1.2f, FMath::Clamp(DamagePercent * 0.5f, 0.18f, 0.55f));
		UDFCameraShakeFunctionLibrary::PlayBossSlamOnOwner(this, PC);
		SetHitVignette(
			FMath::Clamp(DamagePercent * 0.75f, 0.25f, 0.75f),
			FLinearColor(0.9f, 0.f, 0.f, 1.f),
			HitStopSync);
		break;
	default: break;
	}
}

void UDFScreenEffectsComponent::FlashScreen(const FLinearColor Color, const float Duration, const float Intensity)
{
	FlashDuration = FMath::Max(0.01f, Duration);
	FlashTimeRemaining = FlashDuration;
	CurrentFlashColor = Color;
	CurrentFlash = FMath::Clamp(Intensity, 0.f, 1.f);
	SetFlash(CurrentFlash, CurrentFlashColor);
}

void UDFScreenEffectsComponent::ChromaticAberrationPulse(const float Duration, const float Intensity)
{
	ChromaInit = FMath::Max(0.f, Intensity);
	ChromaInitDuration = FMath::Max(0.01f, Duration);
	ChromaTimeRemaining = ChromaInitDuration;
	SetChroma(ChromaInit);
}
