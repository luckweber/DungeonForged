// Source/DungeonForged/Public/Dungeon/Traps/ADFDartProjectile.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ADFDartProjectile.generated.h"

class UProjectileMovementComponent;
class UStaticMeshComponent;
class ADFTrapBase;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFDartProjectile : public AActor
{
	GENERATED_BODY()

public:
	ADFDartProjectile();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Traps|Dart")
	TObjectPtr<UStaticMeshComponent> DartMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Traps|Dart")
	TObjectPtr<UProjectileMovementComponent> ProjectileMove = nullptr;

	/** Filled by the trap on spawn. */
	UPROPERTY(BlueprintReadWrite, Transient, Category = "DF|Traps|Dart|GAS")
	TWeakObjectPtr<ADFTrapBase> OwningTrap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Dart|GAS", meta = (ClampMin = "0.0"))
	float HitDamage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Dart|GAS", meta = (ClampMin = "0.0"))
	float SlowDurationSeconds = 2.f;

	/** Optional instigator (tripwire pawns). */
	UPROPERTY(BlueprintReadWrite, Transient, Category = "DF|Traps|Dart|GAS")
	TWeakObjectPtr<AActor> EffectInstigator;

	void FireInDirection(const FVector& Dir);

protected:
	void BeginPlay() override;
	UFUNCTION()
	void OnImpact(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector Impulse, const FHitResult& Hit);

	/** After sticking; prevents multiple hits. */
	bool bDealt = false;
};
