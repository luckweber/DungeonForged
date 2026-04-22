// Source/DungeonForged/Public/Combat/DFFireballProjectile.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DFFireballProjectile.generated.h"

class UAbilitySystemComponent;
class UNiagaraComponent;
class UProjectileMovementComponent;
class UGameplayEffect;
class USphereComponent;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFFireballProjectile : public AActor
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

	virtual void BeginPlay() override;

protected:
	UFUNCTION()
	void OnHit(
		UPrimitiveComponent* HitComponent, AActor* Other, UPrimitiveComponent* OtherComp, FVector Impulse, const FHitResult& Hit);

	void ApplyFireDamageTo(AActor* Target, const FHitResult& Hit);
};
