// Source/DungeonForged/Public/Localization/DFAccessibilityData.h
#pragma once

#include "CoreMinimal.h"
#include "DFAccessibilityData.generated.h"

UENUM(BlueprintType)
enum class EDFColorBlindMode : uint8
{
	Off UMETA(DisplayName = "Off"),
	Protanopia UMETA(DisplayName = "Protanopia"),
	Deuteranopia UMETA(DisplayName = "Deuteranopia"),
	Tritanopia UMETA(DisplayName = "Tritanopia"),
};

USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFAccessibilitySettings
{
	GENERATED_BODY()

	/** Multiplier applied with project UI scale (0.8–2.0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Accessibility", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float UIFontScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Accessibility")
	bool bHighContrast = false;

	/** Scales down hit-stop, screen FX pulses, and camera-shake amplitudes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Accessibility")
	bool bReduceMotion = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Accessibility")
	bool bColorBlindMode = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Accessibility")
	EDFColorBlindMode ColorBlindType = EDFColorBlindMode::Off;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Accessibility", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MasterVolume = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Accessibility", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MusicVolume = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Accessibility", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SFXVolume = 1.f;

	/** Non-spatial / VO — route to a Voice bus in Blueprints using OnAccessibilitySettingsChanged if needed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Accessibility", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VoiceVolume = 1.f;
};
