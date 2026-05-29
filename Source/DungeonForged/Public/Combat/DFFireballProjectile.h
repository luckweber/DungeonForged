// Source/DungeonForged/Public/Combat/DFFireballProjectile.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Performance/UDFPoolable.h"
#include "DFFireballProjectile.generated.h"

class UDFProjectileSweepComponent;

class UAbilitySystemComponent;
class UNiagaraComponent;
class UProjectileMovementComponent;
class UGameplayEffect;
class USphereComponent;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFFireballProjectile : public AActor, public IUDFPoolable
{
	GENERATED_BODY()

public:
	ADFFireballProjectile();

	/** Instant + execution calc (e.g. GE_FireDamage). Configure in data asset: Instant + UDFamageCalculation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|GAS|Damage")
	TSubclassOf<UGameplayEffect> FireDamageEffect;

	/** Optional: periodic DoT, e.g. 3s duration, 1s period, 3 ticks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|GAS|Damage")
	TSubclassOf<UGameplayEffect> FireDoTEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Components")
	TObjectPtr<USphereComponent> CollisionSphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMove = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|VFX")
	TObjectPtr<UNiagaraComponent> TrailVFX = nullptr;

	/** Optional Niagara system; if not set, component stays hidden. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|VFX")
	TObjectPtr<class UNiagaraSystem> TrailNiagara = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Components")
	TObjectPtr<class UDFProjectileHitTrackerComponent> HitTracker = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Components")
	TObjectPtr<UDFProjectileSweepComponent> ProjectileSweep = nullptr;

	virtual void BeginPlay() override;

	// IUDFPoolable
	virtual void OnAcquiredFromPool() override;
	virtual void OnReleasedToPool() override;
	virtual FName GetPoolName() const override;

protected:
	UFUNCTION()
	void OnHit(
		UPrimitiveComponent* HitComponent, AActor* Other, UPrimitiveComponent* OtherComp, FVector Impulse, const FHitResult& Hit);

	UFUNCTION()
	void OnSweepHit(const FHitResult& Hit, UPrimitiveComponent* SweptComponent);

	void ApplyFireDamageTo(AActor* Target, const FHitResult& Hit);
	void FinishProjectile();
};
