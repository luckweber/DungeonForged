// Source/DungeonForged/Private/Localization/UDFAccessibilitySubsystem.cpp
#include "Localization/UDFAccessibilitySubsystem.h"
#include "Network/UDFNetworkLibrary.h"
#include "Run/DFSaveGame.h"
#include "Run/UDFSaveLibrary.h"
#include "AudioDevice.h"
#include "Camera/CameraComponent.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "FX/UDFScreenEffectsComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/UserInterfaceSettings.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

namespace
{
	void ApplySoundClassVolume(
		UWorld* const W,
		USoundMix* const SoundMix,
		USoundClass* const SoundClass,
		const float Volume)
	{
		if (!W || !SoundMix || !SoundClass)
		{
			return;
		}
		if (const FAudioDeviceHandle H = W->GetAudioDevice())
		{
			if (FAudioDevice* const D = H.GetAudioDevice())
			{
				D->SetSoundMixClassOverride(
					SoundMix,
					SoundClass,
					FMath::Clamp(Volume, 0.f, 1.f),
					1.f,
					0.f,
					true);
			}
		}
	}
}

void UDFAccessibilitySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadSettings();
	ApplySettings(CurrentSettings, false);
}

void UDFAccessibilitySubsystem::LoadSettings()
{
	if (UDFSaveGame* const S = UDFSaveLibrary::ResolveMutableMetaSave(this))
	{
		CurrentSettings = S->AccessibilitySettings;
	}
}

void UDFAccessibilitySubsystem::SaveSettings()
{
	if (UDFSaveGame* S = UDFSaveLibrary::ResolveMutableMetaSave(this))
	{
		S->AccessibilitySettings = CurrentSettings;
		UDFSaveLibrary::SaveMetaSave(this, S);
	}
}

void UDFAccessibilitySubsystem::ApplySettings(const FDFAccessibilitySettings& Settings, const bool bSave)
{
	CurrentSettings = Settings;
	ApplyFontScale();
	ApplyColorBlindPostProcess();
	ApplyHighContrast();
	ApplyAudioVolumes();
	PropagateToPlayerPawns();
	if (bSave)
	{
		SaveSettings();
	}
	OnAccessibilitySettingsChanged.Broadcast(CurrentSettings);
}

void UDFAccessibilitySubsystem::ApplyFontScale() const
{
	const float S = FMath::Clamp(CurrentSettings.UIFontScale, 0.8f, 2.0f);
	if (UUserInterfaceSettings* const UIS = GetMutableDefault<UUserInterfaceSettings>())
	{
		UIS->ApplicationScale = S;
	}
}

void UDFAccessibilitySubsystem::ApplyHighContrast() const
{
	if (ColorBlindRuntimeMID)
	{
		ColorBlindRuntimeMID->SetScalarParameterValue(
			FName("HighContrast"), CurrentSettings.bHighContrast ? 1.f : 0.f);
	}
}

void UDFAccessibilitySubsystem::ApplyAudioVolumes() const
{
	if (UWorld* const W = GetWorld())
	{
		if (const FAudioDeviceHandle H = W->GetAudioDevice())
		{
			if (FAudioDevice* const D = H.GetAudioDevice())
			{
				D->SetTransientPrimaryVolume(FMath::Clamp(CurrentSettings.MasterVolume, 0.f, 1.f));
			}
		}
		ApplySoundClassVolume(W, UserSoundMix, MusicSoundClass, CurrentSettings.MusicVolume);
		ApplySoundClassVolume(W, UserSoundMix, SFXSoundClass, CurrentSettings.SFXVolume);
		ApplySoundClassVolume(W, UserSoundMix, VoiceSoundClass, CurrentSettings.VoiceVolume);
	}
}

void UDFAccessibilitySubsystem::ApplyColorBlindPostProcess()
{
	if (ColorBlindTargetCamera.IsValid() && ColorBlindRuntimeMID)
	{
		ColorBlindTargetCamera->RemoveBlendable(ColorBlindRuntimeMID);
	}
	ColorBlindRuntimeMID = nullptr;
	ColorBlindTargetCamera = nullptr;

	UCameraComponent* Cam = nullptr;
	if (UWorld* const W = GetWorld())
	{
		if (APlayerController* const PC = UDFNetworkLibrary::GetLocalPlayerController(W))
		{
			if (ADFPlayerCharacter* const DFP = Cast<ADFPlayerCharacter>(PC->GetPawn()))
			{
				Cam = DFP->FollowCamera;
			}
			if (!Cam)
			{
				if (APawn* const P = PC->GetPawn())
				{
					Cam = P->FindComponentByClass<UCameraComponent>();
				}
			}
		}
	}

	if (Cam && ColorBlindPostProcessMaterial
		&& CurrentSettings.bColorBlindMode && CurrentSettings.ColorBlindType != EDFColorBlindMode::Off)
	{
		if (UMaterialInstanceDynamic* const MID = UMaterialInstanceDynamic::Create(ColorBlindPostProcessMaterial, Cam))
		{
			const float ModeIdx = static_cast<float>(static_cast<uint8>(CurrentSettings.ColorBlindType));
			MID->SetScalarParameterValue(FName("ColorBlindMode"), ModeIdx);
			MID->SetScalarParameterValue(FName("HighContrast"), CurrentSettings.bHighContrast ? 1.f : 0.f);
			Cam->AddOrUpdateBlendable(MID, 1.f);
			ColorBlindRuntimeMID = MID;
			ColorBlindTargetCamera = Cam;
		}
	}
}

void UDFAccessibilitySubsystem::PropagateToPlayerPawns() const
{
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	for (FConstPlayerControllerIterator It = W->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* const PC = It->Get())
		{
			if (APawn* const Pawn = PC->GetPawn())
			{
				if (UDFScreenEffectsComponent* const FX = Pawn->FindComponentByClass<UDFScreenEffectsComponent>())
				{
					FX->RefreshAccessibilityPresentation();
				}
			}
		}
	}
}

float UDFAccessibilitySubsystem::GetCameraShakeAmplitudeScale() const
{
	if (CurrentSettings.bReduceMotion)
	{
		return 0.1f;
	}
	return FMath::Clamp(CurrentSettings.CameraShakeIntensity, 0.f, 1.f);
}

float UDFAccessibilitySubsystem::GetHitStopIntensityScale() const
{
	if (CurrentSettings.bReduceMotion)
	{
		return 0.1f;
	}
	return FMath::Clamp(CurrentSettings.HitStopIntensity, 0.f, 1.f);
}

float UDFAccessibilitySubsystem::GetVFXIntensityScale() const
{
	return CurrentSettings.bReduceMotion ? 0.1f : 1.f;
}
