// Source/DungeonForged/Public/Combat/ADFKnifeProjectile.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ADFKnifeProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UAbilitySystemComponent;
class UGameplayEffect;
class UNiagaraSystem;
struct FHitResult;

UCLASS()
class DUNGEONFORGED_API ADFKnifeProjectile : public AActor
{
	GENERATED_BODY()

public:
	ADFKnifeProjectile();

	/** Forward speed (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rogue|Knife")
	float FlightSpeed = 2800.f;

	/** If true, OnHit will destroy. */
	UPROPERTY(EditAnywhere, Category = "Rogue|Knife")
	bool bDestroyOnHit = true;

	/** Agility-scaled physical (pre-armor) before Str compensation. */
	UPROPERTY(BlueprintReadWrite, Category = "Rogue|Knife")
	float PhysicalHitDamage = 0.f;

	/** Poison DoT: passed to UGE_DoT_Poison as Data_Damage. */
	UPROPERTY(BlueprintReadWrite, Category = "Rogue|Knife")
	float PoisonMagnitude = 0.f;

	/** CDO defaults; can override. */
	UPROPERTY(EditDefaultsOnly, Category = "Rogue|Knife|GAS")
	TSubclassOf<UGameplayEffect> PhysicalDamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Rogue|Knife|GAS")
	TSubclassOf<UGameplayEffect> PoisonEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Rogue|Knife|VFX")
	TObjectPtr<UNiagaraSystem> ImpactBladeGlintVFX;

protected:
	virtual void BeginPlay() override;
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* Other, UPrimitiveComponent* OtherComp, FVector N, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> Move;
};
