// Source/DungeonForged/Public/Performance/TDFObjectPool.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"

class UWorld;

/**
 * Generic in-memory actor pool (not a UOBJECT). T must derive from AActor.
 * Pooled actors are hidden and collision-disabled while available.
 */
template <typename T>
class TDFObjectPool
{
	static_assert(TIsDerivedFrom<T, AActor>::IsDerived, "TDFObjectPool T must be an AActor");

public:
	void Initialize(const TSubclassOf<T> Class, const int32 InPoolSize, UWorld* InWorld, const FVector SpawnLoc = FVector::ZeroVector, const FRotator SpawnRot = FRotator::ZeroRotator)
	{
		ObjectClass = Class;
		PoolSize = FMath::Max(0, InPoolSize);
		World = InWorld;
		Available.Reset();
		Active.Reset();
		SpawnOffset = SpawnLoc;
		SpawnRotBase = SpawnRot;
		if (!World.IsValid() || !ObjectClass)
		{
			return;
		}
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		P.ObjectFlags |= RF_Transient;
		for (int32 I = 0; I < PoolSize; ++I)
		{
			if (T* const A = World->template SpawnActor<T>(ObjectClass, SpawnOffset, SpawnRotBase, P))
			{
				ResetPooledInstance(A);
				Available.Add(A);
			}
		}
	}

	T* Acquire(const FVector Location, const FRotator Rotation)
	{
		if (!World.IsValid() || !ObjectClass)
		{
			return nullptr;
		}
		T* A = nullptr;
		if (Available.Num() > 0)
		{
			A = Available.Pop(EAllowShrinking::No);
		}
		else
		{
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			P.ObjectFlags |= RF_Transient;
			A = World->template SpawnActor<T>(ObjectClass, Location, Rotation, P);
		}
		if (!A)
		{
			return nullptr;
		}
		A->SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::ResetPhysics);
		A->SetActorHiddenInGame(false);
		SetCollisionEnabledOnActor(A, true);
		A->SetActorTickEnabled(true);
		Active.Add(A);
		return A;
	}

	void Release(T* Object)
	{
		if (!Object)
		{
			return;
		}
		Active.RemoveSingle(Object);
		ResetPooledInstance(Object);
		Available.Add(Object);
	}

	void Prewarm(const int32 Count, const FVector SpawnLoc = FVector::ZeroVector, const FRotator SpawnRot = FRotator::ZeroRotator)
	{
		if (!World.IsValid() || !ObjectClass || Count < 1)
		{
			return;
		}
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		P.ObjectFlags |= RF_Transient;
		for (int32 I = 0; I < Count; ++I)
		{
			if (T* const A = World->template SpawnActor<T>(ObjectClass, SpawnLoc, SpawnRot, P))
			{
				ResetPooledInstance(A);
				Available.Add(A);
			}
		}
		PoolSize += Count;
	}

	int32 GetPoolSize() const { return PoolSize; }
	int32 GetNumActive() const { return Active.Num(); }
	int32 GetNumAvailable() const { return Available.Num(); }

private:
	static void SetCollisionEnabledOnActor(AActor* const InActor, const bool bEnabled)
	{
		if (UPrimitiveComponent* const R = InActor->FindComponentByClass<UPrimitiveComponent>())
		{
			R->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		}
	}

	static void ResetPooledInstance(T* const A)
	{
		if (!A)
		{
			return;
		}
		A->SetActorHiddenInGame(true);
		SetCollisionEnabledOnActor(A, false);
		A->SetActorTickEnabled(false);
	}

	TSubclassOf<T> ObjectClass;
	int32 PoolSize = 0;
	TWeakObjectPtr<UWorld> World;
	FVector SpawnOffset = FVector::ZeroVector;
	FRotator SpawnRotBase = FRotator::ZeroRotator;
	TArray<T*> Available;
	TArray<T*> Active;
};
