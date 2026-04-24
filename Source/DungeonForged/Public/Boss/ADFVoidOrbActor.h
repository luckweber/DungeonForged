// Source/DungeonForged/Public/Boss/ADFVoidOrbActor.h
#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "ADFVoidOrbActor.generated.h"

class ADFBossBase;
class UPrimitiveComponent;
class UDFBossAbility_VoidBarrier;

UCLASS()
class DUNGEONFORGED_API ADFVoidOrbActor : public AActor
{
	GENERATED_BODY()
public:
	ADFVoidOrbActor();
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Bind owner ability; boss supplies damage context. */
	void Init(UDFBossAbility_VoidBarrier* Owning, ADFBossBase* InBoss, const FVector& OrbitOffsetWorld);

	/** Called before force-destroying orbs (timeout / end ability); skips Notify. */
	void Orphan();

	UFUNCTION(BlueprintCallable, Category = "DF|Boss|Void")
	float GetCurrentHealth() const { return Health; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Orbit in XY around boss. */
	void UpdateOrbit(const float Dt);

	UPROPERTY(VisibleAnywhere, Category = "DF|Boss|Void")
	TObjectPtr<USphereComponent> Body;

	/** 150 HP, destroyed by UGameplayStatics::ApplyDamage. */
	UPROPERTY(EditAnywhere, Category = "DF|Boss|Void", meta = (ClampMin = "1.0"))
	float Health = 150.f;

	/** Orbit path speed (radians / sec). */
	UPROPERTY(EditAnywhere, Category = "DF|Boss|Void", meta = (ClampMin = "0.0"))
	float OrbitSpeed = 0.6f;

	UPROPERTY(Transient)
	TObjectPtr<ADFBossBase> Boss;

	UPROPERTY(Transient)
	TObjectPtr<UDFBossAbility_VoidBarrier> OwnerAbility;

	float OrbitAngle = 0.f;
	/** Throttle DoT on same overlap re-entry. */
	float LastTickOverlap = -10000.f;
};
