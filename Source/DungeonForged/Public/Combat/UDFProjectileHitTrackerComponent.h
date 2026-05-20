// Source/DungeonForged/Public/Combat/UDFProjectileHitTrackerComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDFProjectileHitTrackerComponent.generated.h"

/** Prevents duplicate damage when a projectile crosses the same target twice (H6). */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFProjectileHitTrackerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFProjectileHitTrackerComponent();

	/** Returns false if @a Target was already hit by this projectile. */
	UFUNCTION(BlueprintCallable, Category = "DF|Combat|Projectile")
	bool TryRegisterHit(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "DF|Combat|Projectile")
	void ClearHitHistory();

protected:
	UPROPERTY()
	TSet<TWeakObjectPtr<AActor>> HitTargets;
};
