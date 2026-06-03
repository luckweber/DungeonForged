// Source/DungeonForged/Public/FX/UDFScreenEffectsComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "FX/UDFCombatFeedbackTypes.h"
#include "Components/ActorComponent.h"
#include "UDFScreenEffectsComponent.generated.h"

class UPostProcessComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UDFAttributeSet;
class UCameraComponent;
class APlayerController;
class AActor;

/** Higher priority vignette wins when multiple sources write in the same frame. */
UENUM()
enum class EDFScreenVignettePriority : uint8
{
	None = 0,
	LowHealth = 1,
	Spectacle = 2,
	Hit = 3,
	Berserk = 4,
	Death = 5
};

/**
 * Local-player screen post-process (MID or native FPostProcessSettings fallbacks when no master material).
 * Expected MID scalar/vector: VignetteIntensity, VignetteColor, ChromaticAberration, BlurAmount, SaturationMult, FlashIntensity, FlashColor, GrainAmount.
 */
UCLASS(ClassGroup = (DF), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFScreenEffectsComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UDFScreenEffectsComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Call from ADFPlayerCharacter::InitializeGAS when AttributeSet and PC are available (local player). */
	UFUNCTION(BlueprintCallable, Category = "DF|FX|Screen")
	void OnGASReady(UDFAttributeSet* InAttributeSet);

	UFUNCTION(BlueprintCallable, Category = "DF|FX|Screen|Combat")
	void DamageReceived(float DamagePercent);

	UFUNCTION(BlueprintCallable, Category = "DF|FX|Screen|Combat")
	void HealReceived(float HealPercent);

	/** Enable/disable red vignette + film grain + FOV nudge. */
	UFUNCTION(BlueprintCallable, Category = "DF|FX|Screen")
	void BerserkSetActive(bool bActive);

	UFUNCTION(BlueprintCallable, Category = "DF|FX|Screen|Death")
	void OnDeath();

	UFUNCTION(BlueprintCallable, Category = "DF|FX|Screen")
	void TeleportOrBlink();

	UFUNCTION(BlueprintCallable, Category = "DF|FX|Screen|Combat")
	void ApplyHitFromCombat(EDFHitFeedbackBand Band, float DamagePercent, AActor* InstigatorActor, APlayerController* PC);

	UFUNCTION(BlueprintCallable, Category = "DF|FX|Screen|Dodge")
	void ApplyDodgeJuice(float Duration = 0.35f);

	UFUNCTION(BlueprintCallable, Category = "DF|FX|Screen|Spectacle")
	void ApplyKillSpectacle();

	UFUNCTION(BlueprintCallable, Category = "DF|FX|Screen|Spectacle")
	void ApplyRoomClearSpectacle();

	UFUNCTION(BlueprintCallable, Category = "DF|FX|Screen")
	void FlashScreen(FLinearColor Color, float Duration, float Intensity);

	UFUNCTION(BlueprintCallable, Category = "DF|FX|Screen")
	void ChromaticAberrationPulse(float Duration, float Intensity);

	/** Brief radial blur pulse (MID @c BlurAmount or motion-blur fallback). */
	UFUNCTION(BlueprintCallable, Category = "DF|FX|Screen")
	void BlurPulse(float Duration, float Intensity);

	UFUNCTION(BlueprintCallable, Category = "DF|FX|Screen")
	void SetBlurAmount(float Amt);

	UFUNCTION(BlueprintCallable, Category = "DF|FX|Screen|Low Health")
	void LowHealthSetEnabledFromRatio(float Health, float MaxHealth);

	/** Re-evaluate low-health / reduce-motion presentation after accessibility settings change. */
	UFUNCTION(BlueprintCallable, Category = "DF|FX|Screen|Accessibility")
	void RefreshAccessibilityPresentation();

protected:
	/** If set, runtime MID; otherwise native post-process only. */
	UPROPERTY(EditAnywhere, Category = "DF|FX|Screen")
	TObjectPtr<UMaterialInterface> ScreenEffectParentMaterial = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|FX|Screen")
	TObjectPtr<UPostProcessComponent> PostProcessComp = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "DF|FX|Screen")
	TObjectPtr<UMaterialInstanceDynamic> ScreenEffectMaterial = nullptr;

	void SetFlash(float Intensity, const FLinearColor& Color);
	void SetVignette(float Intensity, const FLinearColor& Tint, bool bFromLowHealth = false);
	void SetChroma(float Intensity);
	void SetSaturationMult(float Mult);
	void SetFilmGrain(float Intensity);
	void LerpLocalPlayerFOV(float TargetFOV, float DeltaTime, float InterpSpeed);
	void ResolveAndApplyVignette();
	void SetHitVignette(float Intensity, const FLinearColor& Tint, float Duration);

	void HandleHealthChanged(float NewHealth, float NewMax);
	void HandleOutOfHealth();

	TWeakObjectPtr<UDFAttributeSet> BoundAttributeSet;

	/** Low-health pulse: 0.3 → 0.7 → 0.3 over 1.5s. */
	float LowHealthPhase = 0.f;
	bool bLowHealthVignette = false;
	bool bBerserk = false;
	bool bDeathInProgress = false;
	float DeathFXTime = 0.f;

	float CurrentFlash = 0.f;
	FLinearColor CurrentFlashColor = FLinearColor::Black;
	float FlashTimeRemaining = 0.f;
	float FlashDuration = 0.1f;

	float ChromaTimeRemaining = 0.f;
	float ChromaInit = 0.f;
	float ChromaInitDuration = 0.1f;

	float BlurTimeRemaining = 0.f;
	float BlurInit = 0.f;
	float BlurInitDuration = 0.1f;

	float HealEffectElapsed = 0.f;
	bool bHealEffectActive = false;
	float HealEffectDuration = 0.5f;

	float BaseFOV = 90.f;

	bool bDodgeFOVActive = false;
	float DodgeFOVElapsed = 0.f;
	float DodgeFOVDuration = 0.35f;

	float SpectacleBloomBoost = 0.f;
	float SpectacleBloomTimeRemaining = 0.f;
	float SpectacleFOVTarget = 0.f;
	bool bSpectacleFOVActive = false;
	float SpectacleFOVElapsed = 0.f;
	float SpectacleFOVDuration = 0.f;

	float HitVignetteTimeRemaining = 0.f;
	float HitVignetteDuration = 0.f;
	float HitVignetteIntensity = 0.f;
	FLinearColor HitVignetteTint = FLinearColor(0.5f, 0.f, 0.f, 1.f);
};
