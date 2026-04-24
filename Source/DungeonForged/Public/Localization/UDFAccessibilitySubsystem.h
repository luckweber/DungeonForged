// Source/DungeonForged/Public/Localization/UDFAccessibilitySubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Localization/DFAccessibilityData.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "UDFAccessibilitySubsystem.generated.h"

class UCameraComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDFAccessibilitySettingsChanged, FDFAccessibilitySettings, NewSettings);

UCLASS()
class DUNGEONFORGED_API UDFAccessibilitySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "DF|Accessibility")
	void ApplySettings(const FDFAccessibilitySettings& Settings, bool bSave);

	UFUNCTION(BlueprintCallable, Category = "DF|Accessibility")
	void SaveSettings();

	UFUNCTION(BlueprintCallable, Category = "DF|Accessibility")
	void LoadSettings();

	UFUNCTION(BlueprintPure, Category = "DF|Accessibility")
	const FDFAccessibilitySettings& GetSettings() const { return CurrentSettings; }

	/** When `bReduceMotion` is on: camera shake and similar scale down to this (e.g. 0.1). */
	UFUNCTION(BlueprintPure, Category = "DF|Accessibility")
	float GetCameraShakeAmplitudeScale() const;

	/** Intensity scale for VFX/screen pulse (0.1 when reduce motion). */
	UFUNCTION(BlueprintPure, Category = "DF|Accessibility")
	float GetVFXIntensityScale() const;

	UPROPERTY(BlueprintAssignable, Category = "DF|Accessibility|Events")
	FOnDFAccessibilitySettingsChanged OnAccessibilitySettingsChanged;

	/** If set, applied as a blendable on the local PlayerCameraManager for color vision modes (optional; author in project). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Accessibility|ColorBlind")
	TObjectPtr<UMaterialInterface> ColorBlindPostProcessMaterial = nullptr;

	/** Prevents stacking blendables on repeated Apply. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ColorBlindRuntimeMID = nullptr;

	UPROPERTY(Transient)
	TWeakObjectPtr<UCameraComponent> ColorBlindTargetCamera = nullptr;

protected:
	void ApplyFontScale() const;
	void ApplyHighContrast() const;
	void ApplyAudioVolumes() const;
	void ApplyColorBlindPostProcess();
	void PropagateToPlayerPawns() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Accessibility")
	FDFAccessibilitySettings CurrentSettings;
};
