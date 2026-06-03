// Source/DungeonForged/Public/Combat/DFArcaneMissileProjectile.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Performance/UDFPoolable.h"
#include "DFArcaneMissileProjectile.generated.h"

class UDFProjectileSweepComponent;

class UAbilitySystemComponent;
class UGameplayEffect;
class UProjectileMovementComponent;
class USphereComponent;
class UGameplayAbility;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFArcaneMissileProjectile : public AActor, public IUDFPoolable
{
	GENERATED_BODY()

public:
	ADFArcaneMissileProjectile();

	UPROPERTY(EditAnywhere, Category = "DF|GAS")
	TSubclassOf<UGameplayEffect> MagicDamageEffect;

	UPROPERTY(EditAnywhere, Category = "DF|GAS")
	TSubclassOf<UGameplayEffect> OverloadDamageEffect;

	UPROPERTY(EditAnywhere, Category = "DF|GAS")
	TSubclassOf<UGameplayEffect> SilenceEffect;

	/** Arcane Barrage instance (instanced) for overload bookkeeping. */
	UPROPERTY()
	TObjectPtr<UGameplayAbility> SourceAbility = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> HomingTarget = nullptr;

	UPROPERTY(EditAnywhere, Category = "DF|VFX")
	TObjectPtr<class UNiagaraSystem> HitNiagara = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "DF|Components")
	TObjectPtr<USphereComponent> CollisionSphere = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "DF|Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMove = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "DF|Components")
	TObjectPtr<class UDFProjectileHitTrackerComponent> HitTracker = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "DF|Components")
	TObjectPtr<UDFProjectileSweepComponent> ProjectileSweep = nullptr;

	UFUNCTION(BlueprintCallable, Category = "DF|Projectile")
	void ApplyHomingTarget();

	virtual void OnAcquiredFromPool() override;
	virtual void OnReleasedToPool() override;
	virtual FName GetPoolName() const override;

protected:
	virtual void BeginPlay() override;
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* Other, UPrimitiveComponent* OtherComp, FVector Impulse, const FHitResult& Hit);
	UFUNCTION()
	void OnSweepHit(const FHitResult& Hit, UPrimitiveComponent* SweptComponent);
	void FinishProjectile();
};
