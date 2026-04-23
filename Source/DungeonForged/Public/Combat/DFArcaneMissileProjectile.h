// Source/DungeonForged/Public/Combat/DFArcaneMissileProjectile.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DFArcaneMissileProjectile.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UProjectileMovementComponent;
class USphereComponent;
class UGameplayAbility;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFArcaneMissileProjectile : public AActor
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

protected:
	virtual void BeginPlay() override;
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* Other, UPrimitiveComponent* OtherComp, FVector Impulse, const FHitResult& Hit);
};
