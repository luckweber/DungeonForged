// Source/DungeonForged/Public/Combat/UDFCombatCrowdControlComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FX/UDFCombatFeedbackTypes.h"
#include "UDFCombatCrowdControlComponent.generated.h"

class UDFHitReactionComponent;
class UDFStaggerComponent;

/** Resolved crowd-control layer for one confirmed hit (authority). */
UENUM(BlueprintType)
enum class EDFCrowdControlTier : uint8
{
	FlinchLight		UMETA(DisplayName = "Flinch Light"),
	FlinchHeavy		UMETA(DisplayName = "Flinch Heavy"),
	Knockback		UMETA(DisplayName = "Knockback"),
	PoiseBreak		UMETA(DisplayName = "Poise Break"),
	Juggle			UMETA(DisplayName = "Juggle"),
	Suppressed		UMETA(DisplayName = "Suppressed (CC immune / dead)"),
};

/**
 * Single entry point for victim poise / stagger / hit-react / juggle.
 * Coordinates @c UDFStaggerComponent + @c UDFHitReactionComponent; launcher juggle caps live here.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFCombatCrowdControlComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFCombatCrowdControlComponent();

	/** Damage at or above this uses heavy flinch montages (when not poise-breaking). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|CC|Flinch", meta = (ClampMin = "0.0"))
	float HeavyFlinchDamageThreshold = 30.f;

	/** Prefer @c KnockbackMagnitude when &gt; 0; else damage fallback uses this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|CC|Knockback", meta = (ClampMin = "0.0"))
	float KnockbackMagnitudeThreshold = 60.f;

	/** Legacy damage fallback when @c KnockbackMagnitude is unset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|CC|Knockback", meta = (ClampMin = "0.0"))
	float KnockbackDamageFallbackThreshold = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|CC|Juggle", meta = (ClampMin = "0"))
	int32 MaxJuggleHits = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|CC|Juggle", meta = (ClampMin = "0.1"))
	float JuggleCountResetSeconds = 2.5f;

	/** While juggled, suppress knockback impulses so targets stay airborne. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|CC|Juggle")
	bool bSuppressKnockbackWhileJuggled = true;

	UPROPERTY(BlueprintReadOnly, Category = "DF|CC|Juggle")
	int32 JuggleHitCount = 0;

	UFUNCTION(BlueprintPure, Category = "DF|CC|Juggle")
	bool IsJuggled() const { return bIsJuggled; }

	UFUNCTION(BlueprintPure, Category = "DF|CC|Juggle")
	bool CanReceiveLaunch() const;

	/**
	 * Authority path from @c UDFCombatFeedbackLibrary::DispatchOnHitConfirmed.
	 * Returns the tier applied for logging / style hooks.
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|CC")
	EDFCrowdControlTier ProcessCombatHit(const FDFHitConfirmedContext& Context);

	/**
	 * Launcher gate + physics. Called by @c UDFLauncherComponent before applying velocity.
	 * @return false when juggle cap reached or target invalid.
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|CC|Juggle")
	bool TryReceiveLaunch(
		FVector LaunchVelocity,
		float TargetGravityScale,
		float HangtimeSeconds,
		AActor* Instigator = nullptr);

	UFUNCTION(BlueprintCallable, Category = "DF|CC|Juggle")
	void EndJuggle();

	/** Called when victim touches ground after a juggle. */
	UFUNCTION(BlueprintCallable, Category = "DF|CC|Juggle")
	void OnOwnerLanded();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void ApplyTuningFromDataAsset();
	void CacheSiblingComponents();
	EDFCrowdControlTier ResolveTier(const FDFHitConfirmedContext& Context) const;
	void SyncJuggleTags(bool bActive);
	void ScheduleJuggleCountReset();
	void RestoreSavedGravity();

	UPROPERTY(Transient)
	TObjectPtr<UDFHitReactionComponent> HitReaction;

	UPROPERTY(Transient)
	TObjectPtr<UDFStaggerComponent> Stagger;

	bool bIsJuggled = false;
	float SavedGravityScale = -1.f;
	FTimerHandle JuggleResetTimer;
	FTimerHandle GravityRestoreTimer;
};
