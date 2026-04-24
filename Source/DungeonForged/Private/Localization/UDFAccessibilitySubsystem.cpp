// Source/DungeonForged/Private/Localization/UDFAccessibilitySubsystem.cpp
#include "Localization/UDFAccessibilitySubsystem.h"
#include "Run/DFSaveGame.h"
#include "AudioDevice.h"
#include "Camera/CameraComponent.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/UserInterfaceSettings.h"

void UDFAccessibilitySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadSettings();
	ApplySettings(CurrentSettings, false);
}

void UDFAccessibilitySubsystem::LoadSettings()
{
	if (UDFSaveGame* const S = UDFSaveGame::Load())
	{
		CurrentSettings = S->AccessibilitySettings;
	}
}

void UDFAccessibilitySubsystem::SaveSettings()
{
	if (UDFSaveGame* S = UDFSaveGame::Load())
	{
		S->AccessibilitySettings = CurrentSettings;
		UDFSaveGame::Save(S);
	}
}

void UDFAccessibilitySubsystem::ApplySettings(const FDFAccessibilitySettings& Settings, const bool bSave)
{
	CurrentSettings = Settings;
	ApplyFontScale();
	ApplyHighContrast();
	ApplyAudioVolumes();
	ApplyColorBlindPostProcess();
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
	// Implement palette swap in UMG: listen to OnAccessibilitySettingsChanged in a root widget or use Slate theme assets.
	(void)CurrentSettings.bHighContrast;
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
	}
	(void)CurrentSettings.MusicVolume;
	(void)CurrentSettings.SFXVolume;
	(void)CurrentSettings.VoiceVolume;
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
	if (APlayerController* const PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
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

	if (Cam && ColorBlindPostProcessMaterial
		&& CurrentSettings.bColorBlindMode && CurrentSettings.ColorBlindType != EDFColorBlindMode::Off)
	{
		if (UMaterialInstanceDynamic* const MID = UMaterialInstanceDynamic::Create(ColorBlindPostProcessMaterial, Cam))
		{
			const float ModeIdx = static_cast<float>(static_cast<uint8>(CurrentSettings.ColorBlindType));
			MID->SetScalarParameterValue(FName("ColorBlindMode"), ModeIdx);
			Cam->AddOrUpdateBlendable(MID, 1.f);
			ColorBlindRuntimeMID = MID;
			ColorBlindTargetCamera = Cam;
		}
	}
}

void UDFAccessibilitySubsystem::PropagateToPlayerPawns() const
{
	(void)CurrentSettings;
}

float UDFAccessibilitySubsystem::GetCameraShakeAmplitudeScale() const
{
	return CurrentSettings.bReduceMotion ? 0.1f : 1.f;
}

float UDFAccessibilitySubsystem::GetVFXIntensityScale() const
{
	return CurrentSettings.bReduceMotion ? 0.1f : 1.f;
}
