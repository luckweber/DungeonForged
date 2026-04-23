// Source/DungeonForged/Public/Combat/DFFrostBoltProjectile.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DFFrostBoltProjectile.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UNiagaraComponent;
class UProjectileMovementComponent;
class USphereComponent;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFFrostBoltProjectile : public AActor
{
	GENERATED_BODY()

public:
	ADFFrostBoltProjectile();

	UPROPERTY(EditAnywhere, Category = "DF|GAS")
	TSubclassOf<UGameplayEffect> MagicDamageEffect;

	UPROPERTY(EditAnywhere, Category = "DF|GAS")
	TSubclassOf<UGameplayEffect> FrostSlowEffect;

	UPROPERTY(EditAnywhere, Category = "DF|GAS")
	TSubclassOf<UGameplayEffect> DoTFrostEffect;

	UPROPERTY(EditAnywhere, Category = "DF|GAS")
	TSubclassOf<UGameplayEffect> FreezeEffect;

	/** Optional: lock target for light homing. */
	UPROPERTY()
	TObjectPtr<AActor> HomingTarget = nullptr;

	UPROPERTY(EditAnywhere, Category = "DF|VFX")
	TObjectPtr<class UNiagaraSystem> HitNiagara = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "DF|Components")
	TObjectPtr<USphereComponent> CollisionSphere = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "DF|Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMove = nullptr;

protected:
	virtual void BeginPlay() override;
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* Other, UPrimitiveComponent* OtherComp, FVector Impulse, const FHitResult& Hit);
	void ApplyFrostTo(AActor* Target, const FHitResult& Hit);
};
