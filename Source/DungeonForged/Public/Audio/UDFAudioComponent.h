// Source/DungeonForged/Public/Audio/UDFAudioComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "GameplayTagContainer.h"
#include "UDFAudioComponent.generated.h"

class UDFSoundLibrary;
class USoundAttenuation;
class USoundConcurrency;
class USoundBase;

USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFImpactLayerSound
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<USoundBase> Sound0 = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<USoundBase> Sound1 = nullptr;
};

/**
 * Character/equipment audio. Uses 3D attenuation, concurrency, optional MetaSound
 * (set parameters via ISoundParameterControllerInterface on the component when playing
 * a single UMetaSoundSource from PlaySound* helpers).
 */
UCLASS(ClassGroup = (Audio, DF), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFAudioComponent : public UAudioComponent
{
	GENERATED_BODY()

public:
	UDFAudioComponent();

	/** Binds MS parameter "AbilityIntensity" / "Force" on one-shots; assign on MetaSound. */
	UFUNCTION(BlueprintCallable, Category = "DF|Audio|3D SFX")
	void PlayAbilitySound(const FGameplayTag& AbilityTag, const FVector& Location, float Intensity = 1.f);

	/** Max concurrent footsteps via FootstepConcurrency (e.g. MaxCount=3 in asset). */
	UFUNCTION(BlueprintCallable, Category = "DF|Audio|3D SFX")
	void PlayFootstep(const TEnumAsByte<EPhysicalSurface> Surface, const FVector& Location);

	UFUNCTION(BlueprintCallable, Category = "DF|Audio|3D SFX")
	void PlayImpactSound(
		const TEnumAsByte<EPhysicalSurface> Surface,
		const FVector& Location,
		float ForceMagnitude = 0.5f);

	/** 2D UI. Skipped on dedicated server. (Named PlayDFUISound to avoid clashing with UAudioComponent::PlayUISound.) */
	UFUNCTION(BlueprintCallable, Category = "DF|Audio|UI")
	void PlayDFUISound(USoundBase* InSound, float Volume = 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Audio|Config")
	TObjectPtr<USoundConcurrency> AbilitySFXConcurrency = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Audio|Config")
	TObjectPtr<USoundConcurrency> FootstepConcurrency = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Audio|Config")
	TObjectPtr<USoundAttenuation> DefaultAttenuation3D = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Audio|Map")
	TMap<FGameplayTag, TObjectPtr<USoundBase>> AbilitySoundMap;

	/** If AbilitySoundMap has no key, this library is used when set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Audio|Map")
	TObjectPtr<UDFSoundLibrary> FallbackTagLibrary = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Audio|Map")
	TMap<TEnumAsByte<EPhysicalSurface>, TObjectPtr<USoundBase>> FootstepMap;

	/** If set, a random one is chosen for that surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Audio|Map")
	TMap<TEnumAsByte<EPhysicalSurface>, FDFImpactLayerSound> ImpactBySurface;

protected:
	USoundBase* ResolveAbilitySound(const FGameplayTag& Tag) const;
	USoundBase* ResolveFootstep(const TEnumAsByte<EPhysicalSurface> Surface) const;
	USoundBase* ChooseImpact(const TEnumAsByte<EPhysicalSurface> Surface) const;
	bool ShouldRunAudio() const;
};

/**
 * === MetaSound authoring (Editor; C++ does not build MS graphs) ===
 * MS_Ability_Fireball: input float "Intensity" 0-1. WavePlayer (Fire Whoosh) -> Envelope (Attack=0.01, Release=0.3) -> Spatializer -> Out. Trigger for impact -> MS_Impact_Fire.
 * MS_Ability_FrostBolt: blend WavePlayer (Ice proj) + WavePlayer (ice hum) -> out; impact trigger -> MS_Impact_Ice.
 * MS_Combat_HitFlesh: Random of 4 one-shots + PitchRandom ±0.05 (0.95-1.05) -> out.
 * MS_UI_Purchase: 2D, coin jingle. MS_UI_LevelUp: arpeggio fanfare, 2D. MS_UI_AbilitySelect: whoosh + confirm tone, 2D.
 */
