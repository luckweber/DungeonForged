// Source/DungeonForged/Public/Performance/UDFObjectPoolSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Performance/TDFObjectPool.h"
#include "Subsystems/WorldSubsystem.h"

#include "UDFObjectPoolSubsystem.generated.h"

class AActor;

/**
 * Type-erased interface for a pool backed by TDFObjectPool<AActor subtype>.
 * One UDFObjectPoolSubsystem per world holds all named concrete pools.
 */
class IDFActorObjectPool
{
public:
	virtual ~IDFActorObjectPool() = default;
	virtual AActor* Acquire(const FVector& Location, const FRotator& Rotation) = 0;
	virtual void Release(AActor* Object) = 0;
	virtual void PrewarmPool(int32 Count) = 0;
	virtual int32 GetNumActive() const = 0;
	virtual int32 GetNumAvailable() const = 0;
	virtual int32 GetPoolSize() const = 0;
};

/** Concrete wrapper for a single actor class pool. */
template <typename TActor>
class TDFActorObjectPool : public IDFActorObjectPool
{
	static_assert(TIsDerivedFrom<TActor, AActor>::IsDerived, "TActor must derive from AActor");

public:
	void Initialize(
		const TSubclassOf<TActor> Class, const int32 InPoolSize, UWorld* const InWorld, const FVector SpawnLoc = FVector::ZeroVector,
		const FRotator SpawnRot = FRotator::ZeroRotator)
	{
		Pool.Initialize(Class, InPoolSize, InWorld, SpawnLoc, SpawnRot);
	}

	AActor* Acquire(const FVector& Location, const FRotator& Rotation) override
	{
		return Pool.Acquire(Location, Rotation);
	}

	void Release(AActor* Object) override
	{
		Pool.Release(Cast<TActor>(Object));
	}

	void PrewarmPool(const int32 Count) override
	{
		Pool.Prewarm(Count);
	}

	int32 GetNumActive() const override
	{
		return Pool.GetNumActive();
	}

	int32 GetNumAvailable() const override
	{
		return Pool.GetNumAvailable();
	}

	int32 GetPoolSize() const override
	{
		return Pool.GetPoolSize();
	}

private:
	TDFObjectPool<TActor> Pool;
};

UCLASS()
class DUNGEONFORGED_API UDFObjectPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/** Prewarms default project pools: projectiles, loot (see RegisterDefaultActorPools). */
	UFUNCTION(BlueprintCallable, Category = "DF|Performance|Pools")
	void RegisterDefaultActorPools();

	UFUNCTION(BlueprintCallable, Category = "DF|Performance|Pools")
	AActor* AcquirePooled(FName PoolName, FVector Location, FRotator Rotation);

	UFUNCTION(BlueprintCallable, Category = "DF|Performance|Pools")
	void ReleasePooled(FName PoolName, AActor* Object);

	UFUNCTION(BlueprintCallable, Category = "DF|Performance|Pools")
	void PrewarmPoolByName(FName PoolName, int32 Count);

	UFUNCTION(BlueprintCallable, Category = "DF|Performance|Pools")
	void GetPoolStats(FName PoolName, int32& OutActive, int32& OutAvailable, int32& OutSize) const;

	bool TryGetPool(FName PoolName, TSharedPtr<IDFActorObjectPool>& OutPool) const;

protected:
	/** FName key → e.g. FireballProjectile, FrostBoltProjectile, ArcaneMissile, LootDrop */
	TMap<FName, TSharedPtr<IDFActorObjectPool>> ActorPools;
};
