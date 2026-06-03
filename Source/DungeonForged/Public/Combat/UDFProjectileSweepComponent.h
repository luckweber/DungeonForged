// Source/DungeonForged/Public/Combat/UDFProjectileSweepComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDFProjectileSweepComponent.generated.h"

class USphereComponent;
struct FHitResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectileSweepHit, const FHitResult&, Hit, UPrimitiveComponent*, SweptComponent);

/**
 * Sweeps the projectile root sphere along its motion each tick (anti-tunneling on fast targets).
 * Authority-only. Bind @c OnSweepHit to the same logic as @c OnComponentHit for pawns.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFProjectileSweepComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFProjectileSweepComponent();

	UPROPERTY(BlueprintAssignable, Category = "DF|Projectile")
	FOnProjectileSweepHit OnSweepHit;

	/** Substeps per tick (1–8, matches melee trace tuning). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Projectile", meta = (ClampMin = "1", ClampMax = "8"))
	int32 SweepSubSteps = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Projectile")
	bool bSweepEnabled = true;

	UFUNCTION(BlueprintCallable, Category = "DF|Projectile")
	void ResetTraceSegment();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void PerformSweep(const FVector& Start, const FVector& End);

	UPROPERTY(Transient)
	TObjectPtr<USphereComponent> TrackedSphere;

	FVector LastTraceLocation = FVector::ZeroVector;
	bool bHasLastTraceLocation = false;
};
