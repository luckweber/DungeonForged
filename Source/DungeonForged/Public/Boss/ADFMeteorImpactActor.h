// Source/DungeonForged/Public/Boss/ADFMeteorImpactActor.h
#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "ADFMeteorImpactActor.generated.h"

class UCameraShakeBase;
class UNiagaraSystem;
class ADFBossBase;
class UPrimitiveComponent;

UCLASS()
class DUNGEONFORGED_API ADFMeteorImpactActor : public AActor
{
	GENERATED_BODY()
public:
	ADFMeteorImpactActor();

	/** Call after spawn (authority) so FX and damage use instigator. */
	void InitializeImpact(ADFBossBase* InBoss, const FVector& WorldLocation, float InOuterDamage, float InInnerDamage);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnDelayElapsed();

	void RunImpactOnAuthority();

	UPROPERTY(VisibleAnywhere, Category = "DF|Boss|Meteor")
	TObjectPtr<USphereComponent> HitSphere;

	UPROPERTY(EditAnywhere, Category = "DF|Boss|Meteor")
	TSubclassOf<UCameraShakeBase> CameraShake;

	UPROPERTY(EditAnywhere, Category = "DF|Boss|Meteor")
	TObjectPtr<UNiagaraSystem> ExplosionNiagara;

	UPROPERTY(EditAnywhere, Category = "DF|Boss|Meteor", meta = (ClampMin = "0"))
	float CameraShakeInner = 0.f;

	UPROPERTY(EditAnywhere, Category = "DF|Boss|Meteor", meta = (ClampMin = "0"))
	float CameraShakeOuter = 5000.f;

	/** AOE and outer ring damage. */
	UPROPERTY()
	float OuterDamage = 500.f;

	/** Inner one-shot zone (flat true damage). */
	UPROPERTY()
	float InnerDamage = 1000.f;

	/** Stun to player. */
	UPROPERTY(EditAnywhere, Category = "DF|Boss|Meteor", meta = (ClampMin = "0"))
	float StunDuration = 2.f;

	/** World XY radius: outer (cm). */
	UPROPERTY()
	float ZoneRadius = 400.f;

	/** Inner death zone. */
	UPROPERTY()
	float InnerRadius = 150.f;

	UPROPERTY()
	TObjectPtr<ADFBossBase> BossSource;
};
