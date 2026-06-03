// Source/DungeonForged/Public/Combat/UDFProjectilePoolLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UDFProjectilePoolLibrary.generated.h"

class AActor;
class APawn;
class UWorld;

UCLASS()
class DUNGEONFORGED_API UDFProjectilePoolLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "DF|Projectile|Pool")
	static FName GetPoolNameForProjectileClass(TSubclassOf<AActor> ProjectileClass);

	/** Acquire from pool when registered; otherwise spawns a transient actor. */
	UFUNCTION(BlueprintCallable, Category = "DF|Projectile|Pool", meta = (WorldContext = "WorldContextObject"))
	static AActor* AcquireProjectile(
		UObject* WorldContextObject,
		TSubclassOf<AActor> ProjectileClass,
		FTransform SpawnTransform,
		AActor* Owner,
		APawn* Instigator);

	/** Returns to pool when poolable; otherwise destroys. */
	UFUNCTION(BlueprintCallable, Category = "DF|Projectile|Pool", meta = (WorldContext = "WorldContextObject"))
	static void FinishProjectile(UObject* WorldContextObject, AActor* Projectile);
};
