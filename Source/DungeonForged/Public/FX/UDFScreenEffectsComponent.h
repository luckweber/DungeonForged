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

	UFUNCTION(BlueprintCallable, Category = "DF|FX|Screen")
	void FlashScreen(FLinearColor Color, float Duration, float Intensity);

	UFUNCTION(BlueprintCallable, Category = "DF|FX|Screen")
	void ChromaticAberrationPulse(float Duration, float Intensity);

	UFUNCTION(BlueprintCallable, Category = "DF|FX|Screen|Low Health")
	void LowHealthSetEnabledFromRatio(float Health, float MaxHealth);

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
	void SetBlurAmount(float Amt);
	void SetSaturationMult(float Mult);
	void SetFilmGrain(float Intensity);
	void LerpLocalPlayerFOV(float TargetFOV, float DeltaTime, float InterpSpeed);

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

	float HealEffectElapsed = 0.f;
	bool bHealEffectActive = false;
	float HealEffectDuration = 0.5f;

	float BaseFOV = 90.f;
};
