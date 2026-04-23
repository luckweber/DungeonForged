// Source/DungeonForged/Public/Combat/ADFSmokeBombActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ADFSmokeBombActor.generated.h"

class USphereComponent;
class UNiagaraComponent;
class UGameplayEffect;

UCLASS()
class DUNGEONFORGED_API ADFSmokeBombActor : public AActor
{
	GENERATED_BODY()

public:
	ADFSmokeBombActor();

protected:
	virtual void BeginPlay() override;
	void TickDamage();

	UPROPERTY(VisibleAnywhere, Category = "Rogue|Smoke")
	TObjectPtr<USphereComponent> Overlap;

	UPROPERTY(VisibleAnywhere, Category = "Rogue|Smoke|VFX")
	TObjectPtr<UNiagaraComponent> SmokeVFX;

	UPROPERTY()
	FTimerHandle TimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Rogue|Smoke|GAS")
	TSubclassOf<UGameplayEffect> BlindDebuff;

	UPROPERTY(EditDefaultsOnly, Category = "Rogue|Smoke|GAS")
	TSubclassOf<UGameplayEffect> PlayerSmokeCover;

	/** Cylinder radius in cm. */
	UPROPERTY(EditAnywhere, Category = "Rogue|Smoke")
	float SmokeRadius = 300.f;

	/** How often to refresh blind and cover. */
	UPROPERTY(EditAnywhere, Category = "Rogue|Smoke")
	float TickInterval = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Rogue|Smoke")
	float LifetimeSeconds = 5.f;
};
