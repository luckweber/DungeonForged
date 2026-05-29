// Source/DungeonForged/Private/Combat/UDFProjectilePoolLibrary.cpp
#include "Combat/UDFProjectilePoolLibrary.h"

#include "Combat/ADFKnifeProjectile.h"
#include "Combat/DFArcaneMissileProjectile.h"
#include "Combat/DFFireballProjectile.h"
#include "Combat/DFFrostBoltProjectile.h"
#include "Engine/World.h"
#include "Performance/UDFObjectPoolSubsystem.h"
#include "Performance/UDFPoolable.h"

namespace DFProjectilePoolNames
{
	static const FName Fireball = FName(TEXT("FireballProjectile"));
	static const FName FrostBolt = FName(TEXT("FrostBoltProjectile"));
	static const FName ArcaneMissile = FName(TEXT("ArcaneMissileProjectile"));
	static const FName Knife = FName(TEXT("KnifeProjectile"));
}

FName UDFProjectilePoolLibrary::GetPoolNameForProjectileClass(const TSubclassOf<AActor> ProjectileClass)
{
	if (!ProjectileClass)
	{
		return NAME_None;
	}
	if (ProjectileClass->IsChildOf(ADFFireballProjectile::StaticClass()))
	{
		return DFProjectilePoolNames::Fireball;
	}
	if (ProjectileClass->IsChildOf(ADFFrostBoltProjectile::StaticClass()))
	{
		return DFProjectilePoolNames::FrostBolt;
	}
	if (ProjectileClass->IsChildOf(ADFArcaneMissileProjectile::StaticClass()))
	{
		return DFProjectilePoolNames::ArcaneMissile;
	}
	if (ProjectileClass->IsChildOf(ADFKnifeProjectile::StaticClass()))
	{
		return DFProjectilePoolNames::Knife;
	}
	return NAME_None;
}

AActor* UDFProjectilePoolLibrary::AcquireProjectile(
	UObject* const WorldContextObject,
	const TSubclassOf<AActor> ProjectileClass,
	const FTransform SpawnTransform,
	AActor* const Owner,
	APawn* const Instigator)
{
	UWorld* const World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	if (!World || !ProjectileClass)
	{
		return nullptr;
	}
	const FName PoolName = GetPoolNameForProjectileClass(ProjectileClass);
	AActor* Spawned = nullptr;
	if (PoolName != NAME_None)
	{
		if (UDFObjectPoolSubsystem* const Pools = World->GetSubsystem<UDFObjectPoolSubsystem>())
		{
			Spawned = Pools->AcquirePooled(
				PoolName, SpawnTransform.GetLocation(), SpawnTransform.Rotator());
		}
	}
	if (!Spawned)
	{
		FActorSpawnParameters Params;
		Params.Owner = Owner;
		Params.Instigator = Instigator;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		Spawned = World->SpawnActor<AActor>(ProjectileClass, SpawnTransform, Params);
		if (IUDFPoolable* const Poolable = Cast<IUDFPoolable>(Spawned))
		{
			Poolable->OnAcquiredFromPool();
		}
	}
	if (Spawned)
	{
		Spawned->SetOwner(Owner);
		Spawned->SetInstigator(Instigator);
	}
	return Spawned;
}

void UDFProjectilePoolLibrary::FinishProjectile(UObject* const WorldContextObject, AActor* const Projectile)
{
	if (!IsValid(Projectile))
	{
		return;
	}
	UWorld* const World = Projectile->GetWorld();
	if (!World)
	{
		Projectile->Destroy();
		return;
	}
	if (const IUDFPoolable* const Poolable = Cast<IUDFPoolable>(Projectile))
	{
		const FName PoolName = Poolable->GetPoolName();
		if (PoolName != NAME_None)
		{
			if (UDFObjectPoolSubsystem* const Pools = World->GetSubsystem<UDFObjectPoolSubsystem>())
			{
				Pools->ReleasePooled(PoolName, Projectile);
				return;
			}
		}
	}
	Projectile->Destroy();
}
