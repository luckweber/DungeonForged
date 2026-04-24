// Source/DungeonForged/Private/Performance/UDFPerformanceSubsystem.cpp
#include "Performance/UDFPerformanceSubsystem.h"
#include "Performance/UDFObjectPoolSubsystem.h"
#include "Performance/UDFRoomCullingComponent.h"
#include "Characters/ADFEnemyBase.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "UI/Combat/UDFCombatTextSubsystem.h"

void UDFPerformanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UDFPerformanceSubsystem::Deinitialize()
{
	PoolRegistry.Empty();
	RoomCullers.Empty();
	Super::Deinitialize();
}

void UDFPerformanceSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// UDFObjectPoolSubsystem::OnWorldBeginPlay pre-registers default actor pools; we mirror names for profiling.
	RegisterPoolEntry(FName(TEXT("FireballProjectile")), true);
	RegisterPoolEntry(FName(TEXT("FrostBoltProjectile")), true);
	RegisterPoolEntry(FName(TEXT("ArcaneMissileProjectile")), true);
	RegisterPoolEntry(FName(TEXT("LootDrop")), true);

	if (UDFCombatTextSubsystem* const Cts = InWorld.GetSubsystem<UDFCombatTextSubsystem>())
	{
		Cts->PoolSize = 30;
		Cts->EnsurePooledWidgets();
		RegisterPoolEntry(FName(TEXT("CombatTextWidget")), true);
	}
}

void UDFPerformanceSubsystem::RegisterPoolEntry(const FName PoolName, const bool bRegistered)
{
	PoolRegistry.FindOrAdd(PoolName) = bRegistered;
}

UDFObjectPoolSubsystem* UDFPerformanceSubsystem::GetObjectPoolSubsystem() const
{
	if (UWorld* const W = GetWorld())
	{
		return W->GetSubsystem<UDFObjectPoolSubsystem>();
	}
	return nullptr;
}

void UDFPerformanceSubsystem::TickProfiling(const float DeltaTime)
{
	if (ProfilingInterval <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	LastProfilingTime += DeltaTime;
	if (LastProfilingTime < ProfilingInterval)
	{
		return;
	}
	LastProfilingTime = 0.f;

	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}

	int32 ActiveNiagara = 0;
	for (TObjectIterator<UNiagaraComponent> It; It; ++It)
	{
		UNiagaraComponent* const N = *It;
		if (N && N->GetWorld() == W && N->IsActive() && N->GetOwner() && !N->GetOwner()->IsActorBeingDestroyed())
		{
			++ActiveNiagara;
		}
	}

	int32 EnemyCount = 0;
	for (TActorIterator<ADFEnemyBase> It(W); It; ++It)
	{
		if (IsValid(*It))
		{
			++EnemyCount;
		}
	}

	UDFObjectPoolSubsystem* const Ops = W->GetSubsystem<UDFObjectPoolSubsystem>();
	int32 PooledActive = 0;
	int32 PooledAvail = 0;
	if (Ops)
	{
		static const TArray<FName> PoolNames = {
			FName(TEXT("FireballProjectile")),
			FName(TEXT("FrostBoltProjectile")),
			FName(TEXT("ArcaneMissileProjectile")),
			FName(TEXT("LootDrop"))
		};
		for (const FName& Pn : PoolNames)
		{
			int32 A, V, S;
			Ops->GetPoolStats(Pn, A, V, S);
			PooledActive += A;
			PooledAvail += V;
		}
	}

#if !UE_BUILD_SHIPPING
	{
		int32 CombatPooled = 0;
		int32 CombatInUse = 0;
		if (UDFCombatTextSubsystem* const Cts = W->GetSubsystem<UDFCombatTextSubsystem>())
		{
			CombatPooled = Cts->GetPooledWidgetCount();
			CombatInUse = Cts->GetInUseWidgetCount();
		}
		UE_LOG(LogTemp, Log,
			TEXT("DF|Perf: NiagaraActive=%d  Enemies=%d  ObjectPool(Active/Avail)=%d/%d  CombatText(Pooled/InUse)=%d/%d"),
			ActiveNiagara, EnemyCount, PooledActive, PooledAvail, CombatPooled, CombatInUse);
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			(uint64)((PTRINT)this) ^ 0xDF01u, 5.f, FColor::Cyan,
			FString::Printf(
				TEXT("DF Perf: Niagara=%d  Enemies=%d  PoolAct=%d PoolFree=%d"), ActiveNiagara, EnemyCount, PooledActive, PooledAvail));
	}
#endif
}

void UDFPerformanceSubsystem::RegisterRoomCulling(UDFRoomCullingComponent* Culling)
{
	if (Culling)
	{
		RoomCullers.AddUnique(Culling);
	}
}

void UDFPerformanceSubsystem::UnregisterRoomCulling(UDFRoomCullingComponent* Culling)
{
	RoomCullers.RemoveAllSwap(
		[Culling](const TWeakObjectPtr<UDFRoomCullingComponent>& P)
		{ return !P.IsValid() || P.Get() == Culling; },
		EAllowShrinking::No);
}

void UDFPerformanceSubsystem::UpdateRoomCulling()
{
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(W, 0);
	if (APlayerController* const PC = UGameplayStatics::GetPlayerController(W, 0))
	{
		if (APawn* P = PC->GetPawn())
		{
			Pawn = P;
		}
	}
	if (!Pawn)
	{
		return;
	}
	const FVector Pl = Pawn->GetActorLocation();
	for (int32 I = RoomCullers.Num() - 1; I >= 0; --I)
	{
		TWeakObjectPtr<UDFRoomCullingComponent>& L = RoomCullers[I];
		if (!L.IsValid())
		{
			RoomCullers.RemoveAt(I, 1, EAllowShrinking::No);
			continue;
		}
		L->UpdateVisibilityForPlayer(Pl);
	}
}

void UDFPerformanceSubsystem::Tick(const float DeltaTime)
{
	TickProfiling(DeltaTime);

	RoomCullAccum += DeltaTime;
	if (RoomCullAccum >= RoomCullInterval)
	{
		RoomCullAccum = 0.f;
		UpdateRoomCulling();
	}
}

TStatId UDFPerformanceSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UDFPerformanceSubsystem, STATGROUP_Tickables);
}
