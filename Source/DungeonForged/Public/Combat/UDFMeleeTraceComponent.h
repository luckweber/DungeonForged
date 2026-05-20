// Source/DungeonForged/Public/Combat/UDFMeleeTraceComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "UDFMeleeTraceComponent.generated.h"

class UAbilitySystemComponent;
struct FHitResult;
class UDFHitReactionComponent;
class UGameplayEffect;
class USkeletalMeshComponent;
class UAnimMontage;
class USoundBase;
class UNiagaraSystem;

/** Per-swing attack/impact cosmetics (usually set from UDFAbility_Warrior_MeleeSwing). */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFMeleeTraceCosmetics
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|FX")
	TObjectPtr<USoundBase> SwingSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|FX")
	TObjectPtr<UNiagaraSystem> SwingVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|FX")
	FName SwingFXSocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|FX")
	FVector SwingVFXScale = FVector(1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|FX")
	TObjectPtr<USoundBase> ImpactSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|FX")
	TObjectPtr<UNiagaraSystem> ImpactVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|FX")
	FVector ImpactVFXScale = FVector(1.f);
};

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFMeleeTraceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFMeleeTraceComponent();

	/** Socket on weapon / hand mesh. */
	UPROPERTY(EditAnywhere, Category = "Combat|Trace", meta = (DisplayName = "Trace Start Socket"))
	FName TraceStartSocket = FName("weapon_start");

	/** End of the swing. */
	UPROPERTY(EditAnywhere, Category = "Combat|Trace", meta = (DisplayName = "Trace End Socket"))
	FName TraceEndSocket = FName("weapon_end");

	/** Swept sphere radius (cm). */
	UPROPERTY(EditAnywhere, Category = "Combat|Trace", meta = (ClampMin = "0.0"))
	float TraceRadius = 20.f;

	/**
	 * If socket mid-point is farther than this from the owner, sockets are treated as stale
	 * (common when Mesh_Weapon leader-pose has not updated) and a hand/forward fallback segment is used.
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Trace", meta = (ClampMin = "50.0"))
	float MaxTraceSocketDistanceFromOwner = 350.f;

	/** Blade reach (cm) for hand/forward fallback when weapon sockets are invalid or too far from the owner. */
	UPROPERTY(EditAnywhere, Category = "Combat|Trace", meta = (ClampMin = "10.0"))
	float FallbackReachCm = 120.f;

	/** If empty, `GetOwner()`'s ACharacter->GetMesh() is used. */
	UPROPERTY(EditAnywhere, Category = "Combat|Trace")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	/**
	 * SetByCaller tag used in BuildDamageSpec (default Data.Damage).
	 * The gameplay effect UDFDamageCalculation (GE_MeleeDamage) should read the same SetByCaller key.
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|GAS", meta = (Categories = "Data"))
	FGameplayTag DamageTag;

	UPROPERTY(EditAnywhere, Category = "Combat|GAS", meta = (Categories = "Data"))
	FGameplayTag KnockbackTag;

	/** Instant GE: execution UDFDamageCalculation, SetByCaller Data.Damage / Data.Knockback. */
	UPROPERTY(EditAnywhere, Category = "Combat|GAS")
	TSubclassOf<UGameplayEffect> MeleeDamageGameplayEffect;

	/**
	 * Optional. If the target is below this fraction of max health (before the hit applies),
	 * an extra effect is applied (e.g. execution bonus).
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|GAS", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FinishingHealthFractionThreshold = 0.2f;

	/** Optional instant (or has-duration) GE for finishing hits when health < threshold. */
	UPROPERTY(EditAnywhere, Category = "Combat|GAS")
	TSubclassOf<UGameplayEffect> FinishingBlowGameplayEffect;

	/** SetByCaller magnitude key on FinishingBlow (e.g. extra damage as Data.Damage on that GE’s execution). */
	UPROPERTY(EditAnywhere, Category = "Combat|GAS", meta = (Categories = "Data"))
	FGameplayTag FinishingSetByCallerTag;

	/** Value passed to SetByCaller for finishing GE when the condition is met. */
	UPROPERTY(EditAnywhere, Category = "Combat|GAS")
	float FinishingSetByCallerMagnitude = 20.f;

	/**
	 * Applied to the target when it is struck while carrying `State.Combat.ParryWindow.Open`
	 * (set by `UANS_DFParryWindow`). Author as a stun GE with a SetByCaller bonus-damage tag.
	 * Normal damage is still applied — parry is *additive*.
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Parry")
	TSubclassOf<UGameplayEffect> ParryReactionGameplayEffect;

	/** SetByCaller key on the parry GE (typically `Data.Damage` for bonus damage on the reaction). */
	UPROPERTY(EditAnywhere, Category = "Combat|Parry", meta = (Categories = "Data"))
	FGameplayTag ParrySetByCallerTag;

	/** Value sent for `ParrySetByCallerTag`. */
	UPROPERTY(EditAnywhere, Category = "Combat|Parry", meta = (ClampMin = "0.0"))
	float ParrySetByCallerMagnitude = 30.f;

	/** Damage multiplier applied to the *normal* swing spec when parry triggers (1.5x default = chunky AAA feel). */
	UPROPERTY(EditAnywhere, Category = "Combat|Parry", meta = (ClampMin = "1.0"))
	float ParryDamageMultiplier = 1.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace")
	bool bTracing = false;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Combat|Trace")
	TArray<TWeakObjectPtr<AActor>> HitActorsThisSwing;

	/** Result of the last BuildDamageSpec call (for debugging / BP). */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|GAS")
	FGameplayEffectSpecHandle CachedDamageSpec;

	UPROPERTY(EditAnywhere, Category = "Combat|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	/** Default damage when BuildDamageSpec is used without overrides. */
	UPROPERTY(EditAnywhere, Category = "Combat|Damage", meta = (ClampMin = "0.0"))
	float BaseDamage = 25.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Damage", meta = (ClampMin = "0.0"))
	float BaseKnockback = 500.f;

	/** Only the authority runs traces that apply GAS/physics. */
	UPROPERTY(EditAnywhere, Category = "Combat|Net")
	bool bServerOnlyTraces = true;

	/** When false, WorldDynamic overlaps (e.g. loot pickup spheres) are ignored by melee queries. */
	UPROPERTY(EditAnywhere, Category = "Combat|Trace")
	bool bIncludeWorldDynamicInTraceQuery = false;

	/** Draw sweep for debugging. */
	UPROPERTY(EditAnywhere, Category = "Combat|Debug")
	bool bDrawDebugTrace = false;

	/** Extra Output Log detail (weapon mesh, sweep hits, enemy collision). Toggle with df.meleedebug verbose. */
	UPROPERTY(EditAnywhere, Category = "Combat|Debug")
	bool bVerboseTraceLog = false;

	/** One-shot dump: weapon mesh, sockets, nearest enemies, collision (df.meleedebug dump). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Debug")
	void DumpMeleeTraceDiagnostics() const;

	/** Clears per-swing tracking and sets bTracing. Rebuilds CachedDamageSpec. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Trace")
	void StartTrace();

	UFUNCTION(BlueprintCallable, Category = "Combat|Trace")
	void EndTrace();

	/**
	 * Server-only fallback: start/end trace at AN_TraceStart / AN_TraceEnd times on the montage
	 * (when anim notifies do not fire under LocalPredicted GAS montages).
	 */
	void ScheduleAuthorityTraceWindowsFromMontage(UAnimMontage* Montage, float PlayRate = 1.f);

	void ClearScheduledTraceWindows();

	/** Called from Tick while bTracing. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Trace")
	void TickTrace(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Combat|GAS")
	FGameplayEffectSpecHandle BuildDamageSpec(float BaseDamageValue, float KnockbackForce);

	/** C++: OptionalHit for impact on UDFHitReactionComponent. UFUNCTION cannot take FHitResult* — use BP overload without hit, or call from C++. */
	void ApplyDamageToTarget(AActor* Target, const FGameplayEffectSpecHandle& SpecHandle, const FHitResult* OptionalHit = nullptr);

	/** Blueprint: no impact location; VFX in hit reaction use mesh fallback. */
	UFUNCTION(BlueprintCallable, Category = "Combat|GAS", meta = (DisplayName = "ApplyDamageToTarget"))
	void ApplyDamageToTargetBP(AActor* Target, FGameplayEffectSpecHandle SpecHandle);

	/** Exposed for AnimNotify: StartTrace. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Trace")
	void SetBaseDamageForNextSwing(float NewBaseDamage) { OverrideBaseDamage = NewBaseDamage; bUseOverrideBaseDamage = true; }

	UFUNCTION(BlueprintCallable, Category = "Combat|Trace")
	void SetBaseKnockbackForNextSwing(float NewKnockback) { OverrideBaseKnockback = NewKnockback; bUseOverrideKnockback = true; }

	/** Next swing uses heavy damage/knockback multipliers and wider trace. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Trace|Heavy")
	void ConfigureHeavySwing(float DamageMultiplier, float KnockbackMultiplier, float TraceRadiusBonus);

	/** Default cosmetics on the component; overridden per swing via SetSwingCosmetics. */
	UPROPERTY(EditAnywhere, Category = "Combat|FX")
	FDFMeleeTraceCosmetics DefaultCosmetics;

	/** Called from melee GA on activate (server). Merged over DefaultCosmetics for the next trace window. */
	UFUNCTION(BlueprintCallable, Category = "Combat|FX")
	void SetSwingCosmetics(const FDFMeleeTraceCosmetics& Cosmetics);

	UFUNCTION(BlueprintCallable, Category = "Combat|Trace|Heavy")
	void ClearHeavySwing();

	UFUNCTION(BlueprintPure, Category = "Combat|Trace|Heavy")
	float GetEffectiveTraceRadius() const;

	/** Mesh used for socket-based traces (assigned weapon skeletal or owner fallback); for tooling / debug dumps. */
	USkeletalMeshComponent* GetResolvedTraceMesh() const;

	/** Human-readable trace mesh + socket span for `df.meleedebug` / logs (development). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Debug")
	FString GetMeleeTraceDiagnosticString() const;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void BeginPlay() override;

	/** Resolve SkeletalMesh from owner if unset. */
	USkeletalMeshComponent* GetMesh() const;

	void ProcessHitResults(TArray<FHitResult>& Hits, UWorld* World);
	bool TryMeleeOverlapFallback(UWorld* World, AActor* Owner, const FVector& Center, float Radius);

	FCollisionObjectQueryParams BuildMeleeTraceObjectQuery() const;
	/** Returns false when no valid segment could be built. */
	bool ResolveTraceSegmentWorld(FVector& OutStart, FVector& OutEnd);
	bool TryBuildFallbackTraceSegment(FVector& OutStart, FVector& OutEnd) const;
	bool TrySegmentFromBodyMesh(AActor* Owner, FVector& OutStart, FVector& OutEnd) const;
	USkeletalMeshComponent* GetOwnerBodyMesh() const;
	static bool IsTraceSegmentNearOwner(const AActor* Owner, const FVector& A, const FVector& B, float MaxDistFromOwner);

	bool ShouldLogMeleeTraceDetail() const;
	void LogMeleeTraceWeaponAndSockets() const;
	void LogMeleeTraceNearestTargets(const FVector& TraceStart, const FVector& TraceEnd) const;
	void LogActorCollisionForMelee(AActor* Actor, const TCHAR* Label) const;

	bool bUseOverrideBaseDamage = false;
	float OverrideBaseDamage = 0.f;
	bool bUseOverrideKnockback = false;
	float OverrideBaseKnockback = 0.f;

	FTimerHandle ScheduledTraceStartTimer;
	FTimerHandle ScheduledTraceEndTimer;

	FVector LastTraceStartWS = FVector::ZeroVector;
	FVector LastTraceEndWS = FVector::ZeroVector;
	bool bHasLastTraceSegment = false;

	bool bLastTraceUsedFallbackSegment = false;

	bool bHeavySwingActive = false;
	float HeavySwingTraceRadius = 0.f;

	FDFMeleeTraceCosmetics ActiveCosmetics;

	void PlaySwingCosmetics() const;
	void PlayImpactCosmeticsAt(const FVector& HitLocation, const FVector& HitNormal) const;
};
