// Source/DungeonForged/Private/Performance/UDFObjectPoolSubsystem.cpp
#include "Performance/UDFObjectPoolSubsystem.h"
#include "Combat/DFArcaneMissileProjectile.h"
#include "Combat/DFFireballProjectile.h"
#include "Combat/DFFrostBoltProjectile.h"
#include "DFLootDrop.h"
#include "Engine/World.h"

namespace DungeonForgedPoolNames
{
	static const FName Fireball = FName(TEXT("FireballProjectile"));
	static const FName FrostBolt = FName(TEXT("FrostBoltProjectile"));
	static const FName ArcaneMissile = FName(TEXT("ArcaneMissileProjectile"));
	static const FName LootDrop = FName(TEXT("LootDrop"));
}

void UDFObjectPoolSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	RegisterDefaultActorPools();
}

void UDFObjectPoolSubsystem::RegisterDefaultActorPools()
{
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	ActorPools.Empty();

	{
		TSharedPtr<TDFActorObjectPool<ADFFireballProjectile>> P = MakeShared<TDFActorObjectPool<ADFFireballProjectile>>();
		P->Initialize(TSubclassOf<ADFFireballProjectile>(ADFFireballProjectile::StaticClass()), 20, W);
		ActorPools.Add(DungeonForgedPoolNames::Fireball, P);
	}
	{
		TSharedPtr<TDFActorObjectPool<ADFFrostBoltProjectile>> P = MakeShared<TDFActorObjectPool<ADFFrostBoltProjectile>>();
		P->Initialize(TSubclassOf<ADFFrostBoltProjectile>(ADFFrostBoltProjectile::StaticClass()), 20, W);
		ActorPools.Add(DungeonForgedPoolNames::FrostBolt, P);
	}
	{
		TSharedPtr<TDFActorObjectPool<ADFArcaneMissileProjectile>> P = MakeShared<TDFActorObjectPool<ADFArcaneMissileProjectile>>();
		P->Initialize(TSubclassOf<ADFArcaneMissileProjectile>(ADFArcaneMissileProjectile::StaticClass()), 40, W);
		ActorPools.Add(DungeonForgedPoolNames::ArcaneMissile, P);
	}
	{
		TSharedPtr<TDFActorObjectPool<ADFLootDrop>> P = MakeShared<TDFActorObjectPool<ADFLootDrop>>();
		P->Initialize(TSubclassOf<ADFLootDrop>(ADFLootDrop::StaticClass()), 30, W);
		ActorPools.Add(DungeonForgedPoolNames::LootDrop, P);
	}
}

AActor* UDFObjectPoolSubsystem::AcquirePooled(const FName PoolName, const FVector Location, const FRotator Rotation)
{
	if (TSharedPtr<IDFActorObjectPool> P; TryGetPool(PoolName, P))
	{
		return P->Acquire(Location, Rotation);
	}
	return nullptr;
}

void UDFObjectPoolSubsystem::ReleasePooled(const FName PoolName, AActor* Object)
{
	if (TSharedPtr<IDFActorObjectPool> P; TryGetPool(PoolName, P))
	{
		P->Release(Object);
	}
}

void UDFObjectPoolSubsystem::PrewarmPoolByName(const FName PoolName, const int32 Count)
{
	if (TSharedPtr<IDFActorObjectPool> P; TryGetPool(PoolName, P))
	{
		P->PrewarmPool(Count);
	}
}

void UDFObjectPoolSubsystem::GetPoolStats(const FName PoolName, int32& OutActive, int32& OutAvailable, int32& OutSize) const
{
	OutActive = OutAvailable = OutSize = 0;
	if (TSharedPtr<IDFActorObjectPool> P; TryGetPool(PoolName, P))
	{
		OutActive = P->GetNumActive();
		OutAvailable = P->GetNumAvailable();
		OutSize = P->GetPoolSize();
	}
}

bool UDFObjectPoolSubsystem::TryGetPool(const FName PoolName, TSharedPtr<IDFActorObjectPool>& OutPool) const
{
	if (const TSharedPtr<IDFActorObjectPool>* F = ActorPools.Find(PoolName))
	{
		OutPool = *F;
		return OutPool.IsValid();
	}
	return false;
}
