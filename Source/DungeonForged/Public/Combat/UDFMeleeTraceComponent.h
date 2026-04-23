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

	/** Draw sweep for debugging. */
	UPROPERTY(EditAnywhere, Category = "Combat|Debug")
	bool bDrawDebugTrace = false;

	/** Clears per-swing tracking and sets bTracing. Rebuilds CachedDamageSpec. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Trace")
	void StartTrace();

	UFUNCTION(BlueprintCallable, Category = "Combat|Trace")
	void EndTrace();

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

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void BeginPlay() override;

	/** Resolve SkeletalMesh from owner if unset. */
	USkeletalMeshComponent* GetMesh() const;

	bool bUseOverrideBaseDamage = false;
	float OverrideBaseDamage = 0.f;
	bool bUseOverrideKnockback = false;
	float OverrideBaseKnockback = 0.f;
};
