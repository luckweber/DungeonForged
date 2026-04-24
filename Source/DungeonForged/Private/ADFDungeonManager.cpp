// Source/DungeonForged/Private/ADFDungeonManager.cpp

#include "ADFDungeonManager.h"
#include "UI/Minimap/ADFMinimapRoom.h"
#include "Characters/ADFPlayerState.h"
#include "Characters/ADFEnemyBase.h"
#include "UI/UDFAbilitySelectionSubsystem.h"
#include "Data/PCGPointData.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "PCGComponent.h"
#include "PCGData.h"
#include "TimerManager.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

void UDFDungeonManager::Deinitialize()
{
	UnbindPCG();
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(PCGFallbackTimer);
	}
	RegisteredMinimapRooms.Reset();
	CurrentPlayerMinimapRoom = nullptr;
	ClearFloorActors();
	Super::Deinitialize();
}

void UDFDungeonManager::RegisterMinimapRoom(ADFMinimapRoom* const Room)
{
	if (Room)
	{
		RegisteredMinimapRooms.AddUnique(Room);
	}
}

void UDFDungeonManager::UnregisterMinimapRoom(ADFMinimapRoom* const Room)
{
	if (Room)
	{
		RegisteredMinimapRooms.Remove(Room);
	}
	if (CurrentPlayerMinimapRoom == Room)
	{
		CurrentPlayerMinimapRoom = nullptr;
	}
}

void UDFDungeonManager::NotifyMinimapRoomRevealed(ADFMinimapRoom* const Room)
{
	if (Room)
	{
		OnRoomRevealed.Broadcast(Room);
	}
}

void UDFDungeonManager::NotifyMinimapRoomVisited(ADFMinimapRoom* const Room)
{
	if (Room)
	{
		OnRoomVisited.Broadcast(Room);
	}
}

void UDFDungeonManager::SetPlayerCurrentMinimapRoom(ADFMinimapRoom* const Room)
{
	if (CurrentPlayerMinimapRoom == Room)
	{
		return;
	}
	CurrentPlayerMinimapRoom = Room;
	OnPlayerMinimapRoomChanged.Broadcast(Room);
}

void UDFDungeonManager::StartFloor(int32 FloorNumber)
{
	if (!IsAuthorityWorld())
	{
		return;
	}

	FDFDungeonFloorRow Row;
	if (!FindFloorRowByNumber(FloorNumber, Row))
	{
		UE_LOG(LogTemp, Warning, TEXT("StartFloor: no dungeon row for floor %d"), FloorNumber);
		return;
	}

	CurrentFloor = FloorNumber;
	CachedCurrentFloorRow = Row;
	bHasCurrentFloorRow = true;
	bFloorCleared = false;

	ClearFloorActors();

	PendingFloorRow = Row;
	bHasPendingFloorRow = true;

	if (Row.bHasBoss)
	{
		OnBossSpawned.Broadcast(Row.BossEnemyRow);
	}

	GenerateDungeon();
}

void UDFDungeonManager::PlaceRoomTemplates_Implementation()
{
}

void UDFDungeonManager::GenerateDungeon()
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("DungeonForged.GenerateDungeon"));
	if (!IsAuthorityWorld())
	{
		return;
	}

	PlaceRoomTemplates();

	UnbindPCG();
	bWaitingForPCG = true;

	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(PCGFallbackTimer);
	}

	if (UPCGComponent* PCG = ResolvePCGComponent())
	{
		PCG->OnPCGGraphGeneratedExternal.AddDynamic(this, &UDFDungeonManager::OnPCGGenerationFinished);
		PCGBoundComponent = PCG;
		PCG->Generate(true);

		// Failsafe if a PCG run never calls back (graph misconfiguration, etc.)
		if (UWorld* W = GetWorld())
		{
			W->GetTimerManager().SetTimer(
				PCGFallbackTimer,
				[this]()
				{
					UnbindPCG();
					FinishPCGAndSpawn();
				},
				10.f,
				false);
		}
	}
	else
	{
		// No component: run spawn immediately
		FinishPCGAndSpawn();
	}
}

void UDFDungeonManager::OnPCGGenerationFinished(UPCGComponent* /*PCG*/)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(PCGFallbackTimer);
	}
	UnbindPCG();
	FinishPCGAndSpawn();
}

void UDFDungeonManager::FinishPCGAndSpawn()
{
	bWaitingForPCG = false;
	if (!bHasPendingFloorRow)
	{
		return;
	}

	const FDFDungeonFloorRow RowCopy = PendingFloorRow;
	bHasPendingFloorRow = false;
	SpawnEnemies(RowCopy);
}

void UDFDungeonManager::UnbindPCG()
{
	if (UPCGComponent* P = PCGBoundComponent.Get())
	{
		P->OnPCGGraphGeneratedExternal.RemoveDynamic(this, &UDFDungeonManager::OnPCGGenerationFinished);
	}
	PCGBoundComponent.Reset();
}

UPCGComponent* UDFDungeonManager::ResolvePCGComponent() const
{
	if (IsValid(PCGOwnerActor.Get()))
	{
		return PCGOwnerActor->FindComponentByClass<UPCGComponent>();
	}
	return nullptr;
}

void UDFDungeonManager::CollectSpawnPoints_Implementation(TArray<FTransform>& OutSpawnPoints) const
{
	OutSpawnPoints.Reset();
	if (SpawnPointsPreview.Num() > 0)
	{
		OutSpawnPoints.Append(SpawnPointsPreview);
	}

	if (IsValid(PCGOwnerActor.Get()))
	{
		if (UPCGComponent* PCG = PCGOwnerActor->FindComponentByClass<UPCGComponent>())
		{
			const FPCGDataCollection& Col = PCG->GetGeneratedGraphOutput();
			for (const FPCGTaggedData& Tag : Col.TaggedData)
			{
				if (const UPCGPointData* Pts = Cast<UPCGPointData>(Tag.Data))
				{
					for (const FPCGPoint& Pt : Pts->GetPoints())
					{
						OutSpawnPoints.Add(Pt.Transform);
					}
				}
			}
		}
	}
}

void UDFDungeonManager::SpawnEnemies(const FDFDungeonFloorRow& FloorData)
{
	if (!IsAuthorityWorld())
	{
		return;
	}
	if (!EnemyDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnEnemies: EnemyDataTable is not set"));
		EnemiesRemaining = 0;
		return;
	}

	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<FTransform> SpawnPoints;
	CollectSpawnPoints(SpawnPoints);
	if (SpawnPoints.Num() == 0)
	{
		UE_LOG(
			LogTemp, Warning, TEXT("SpawnEnemies: no spawn points (PCG or SpawnPointsPreview). Spawning at origin as fallback"));
		SpawnPoints.Add(FTransform::Identity);
	}

	const int32 MinC = FMath::Min(FloorData.MinEnemies, FloorData.MaxEnemies);
	const int32 MaxC = FMath::Max(FloorData.MinEnemies, FloorData.MaxEnemies);
	int32 GruntCount = (MinC == MaxC) ? MinC : FMath::RandRange(MinC, MaxC);
	if (FloorData.PossibleEnemyRows.Num() == 0)
	{
		GruntCount = 0;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	int32 PointIndex = 0;
	for (int32 i = 0; i < GruntCount; ++i)
	{
		const FName RowName = PickWeightedRandomEnemyRow(FloorData.PossibleEnemyRows, FloorData);
		if (RowName.IsNone())
		{
			break;
		}
		if (const FDFEnemyTableRow* ER = EnemyDataTable->FindRow<FDFEnemyTableRow>(RowName, TEXT("DFDungeon|SpawnEnemies")))
		{
			if (!ER->EnemyClass)
			{
				continue;
			}
			const FTransform& T = SpawnPoints[PointIndex % SpawnPoints.Num()];
			++PointIndex;
			AActor* const Spawned = World->SpawnActor<AActor>(ER->EnemyClass, T, Params);
			if (Spawned)
			{
				RegisterSpawnedEnemy(Spawned);
				if (ADFEnemyBase* const Char = Cast<ADFEnemyBase>(Spawned))
				{
					Char->InitializeFromDataTable(EnemyDataTable, RowName);
				}
			}
		}
	}

	if (FloorData.bHasBoss && !FloorData.BossEnemyRow.IsNone())
	{
		if (const FDFEnemyTableRow* BossRow = EnemyDataTable->FindRow<FDFEnemyTableRow>(
				FloorData.BossEnemyRow, TEXT("DFDungeon|Boss")))
		{
			if (BossRow->EnemyClass)
			{
				const FTransform& T = SpawnPoints[PointIndex % SpawnPoints.Num()];
				++PointIndex;
				AActor* const Spawned = World->SpawnActor<AActor>(BossRow->EnemyClass, T, Params);
				if (Spawned)
				{
					RegisterSpawnedEnemy(Spawned);
					if (ADFEnemyBase* const Char = Cast<ADFEnemyBase>(Spawned))
					{
						Char->InitializeFromDataTable(EnemyDataTable, FloorData.BossEnemyRow);
					}
				}
			}
		}
	}

	EnemiesRemaining = SpawnedEnemies.Num();
}

void UDFDungeonManager::RegisterSpawnedEnemy(AActor* Enemy)
{
	if (!Enemy)
	{
		return;
	}
	SpawnedEnemies.Add(Enemy);
	if (ADFEnemyBase* const E = Cast<ADFEnemyBase>(Enemy))
	{
		E->OnEnemyDied.AddDynamic(this, &UDFDungeonManager::HandleEnemyDied);
	}
}

void UDFDungeonManager::HandleEnemyDied(AActor* Enemy, AActor* /*Killer*/, float /*ExperienceReward*/)
{
	OnEnemyKilled(Enemy);
}

void UDFDungeonManager::UnregisterEnemy(AActor* Enemy)
{
	if (!Enemy)
	{
		return;
	}
	if (ADFEnemyBase* const E = Cast<ADFEnemyBase>(Enemy))
	{
		E->OnEnemyDied.RemoveDynamic(this, &UDFDungeonManager::HandleEnemyDied);
	}
	if (SpawnedEnemies.Remove(Enemy) > 0)
	{
		EnemiesRemaining = FMath::Max(0, EnemiesRemaining - 1);
	}
}

void UDFDungeonManager::ClearFloorActors()
{
	for (AActor* const A : SpawnedEnemies)
	{
		if (A)
		{
			if (ADFEnemyBase* const E = Cast<ADFEnemyBase>(A))
			{
				E->OnEnemyDied.RemoveDynamic(this, &UDFDungeonManager::HandleEnemyDied);
			}
			A->Destroy();
		}
	}
	SpawnedEnemies.Reset();
	EnemiesRemaining = 0;
}

void UDFDungeonManager::OnEnemyKilled(AActor* Enemy)
{
	if (!IsAuthorityWorld() || !Enemy)
	{
		return;
	}
	if (!SpawnedEnemies.Contains(Enemy))
	{
		return;
	}
	UnregisterEnemy(Enemy);
	if (EnemiesRemaining <= 0)
	{
		PerformFloorCleared();
	}
}

void UDFDungeonManager::OnFloorCleared_OpenExitAndLoot_Implementation()
{
}

void UDFDungeonManager::PerformFloorCleared()
{
	if (!IsAuthorityWorld())
	{
		return;
	}
	if (bFloorCleared)
	{
		return;
	}
	if (EnemiesRemaining > 0)
	{
		return;
	}
	bFloorCleared = true;
	OnFloorCleared_OpenExitAndLoot_Implementation();
	OnFloorCleared.Broadcast();

	// Server-only: roll 1-of-3 and send the same set to all clients. Empty pool → next floor, no UI.
	if (UWorld* const W = GetWorld())
	{
		bFloorOfferResolved = false;
		++ActiveFloorOfferId;
		if (UDFAbilitySelectionSubsystem* const Sub = W->GetSubsystem<UDFAbilitySelectionSubsystem>())
		{
			const TArray<FDFAbilityRolledChoice> Choices = Sub->RollAbilityChoices(3);
			const int32 FloorForUi = CurrentFloor;
			const int32 SkipG = Sub->SkipGoldReward;
			if (Choices.Num() == 0)
			{
				bFloorOfferResolved = true;
				AdvanceToNextFloor();
			}
			else
			{
				for (FConstPlayerControllerIterator It = W->GetPlayerControllerIterator(); It; ++It)
				{
					if (APlayerController* const Pc = It->Get())
					{
						if (ADFPlayerState* const PState = Pc->GetPlayerState<ADFPlayerState>())
						{
							PState->Client_OpenAbilitySelectionScreen(FloorForUi, Choices, SkipG, ActiveFloorOfferId, 30.f);
						}
					}
				}
			}
		}
		else
		{
			bFloorOfferResolved = true;
			AdvanceToNextFloor();
		}
	}
}

void UDFDungeonManager::AdvanceToNextFloor()
{
	if (!IsAuthorityWorld())
	{
		return;
	}
	FDFDungeonFloorRow NextRow;
	if (!FindFloorRowByNumber(CurrentFloor + 1, NextRow))
	{
		OnRunCompleted.Broadcast();
		return;
	}
	StartFloor(CurrentFloor + 1);
}

bool UDFDungeonManager::GetCurrentFloorData(FDFDungeonFloorRow& OutRow) const
{
	if (bHasCurrentFloorRow)
	{
		OutRow = CachedCurrentFloorRow;
		return true;
	}
	return false;
}

void UDFDungeonManager::NotifyRunFailed(AActor* Player)
{
	OnRunFailed.Broadcast(Player);
}

bool UDFDungeonManager::FindFloorRowByNumber(int32 InFloor, FDFDungeonFloorRow& OutRow) const
{
	if (!DungeonFloorTable)
	{
		return false;
	}
	bool bFound = false;
	DungeonFloorTable->ForeachRow<FDFDungeonFloorRow>(
		TEXT("UDFDungeonManager::FindFloorRowByNumber"),
		[InFloor, &bFound, &OutRow](const FName& /*Key*/, const FDFDungeonFloorRow& Value)
		{
			if (bFound)
			{
				return;
			}
			if (Value.FloorNumber == InFloor)
			{
				OutRow = Value;
				bFound = true;
			}
		});
	return bFound;
}

bool UDFDungeonManager::IsAuthorityWorld() const
{
	const UWorld* const World = GetWorld();
	return World && (World->GetNetMode() != NM_Client);
}

float UDFDungeonManager::ComputeSpawnWeight(const FDFEnemyTableRow& EnemyRow, const FDFDungeonFloorRow& FloorRow) const
{
	const float StatSum = FMath::Max(1.f, EnemyRow.BaseHealth + EnemyRow.BaseDamage + EnemyRow.BaseArmor);
	return StatSum * FMath::Max(0.1f, FloorRow.DifficultyMultiplier);
}

FName UDFDungeonManager::PickWeightedRandomEnemyRow(const TArray<FName>& RowNames, const FDFDungeonFloorRow& FloorRow) const
{
	if (RowNames.Num() == 0 || !EnemyDataTable)
	{
		return NAME_None;
	}
	float TotalW = 0.f;
	for (const FName& N : RowNames)
	{
		if (const FDFEnemyTableRow* ER = EnemyDataTable->FindRow<FDFEnemyTableRow>(N, TEXT("DFDungeon|Weight")))
		{
			TotalW += ComputeSpawnWeight(*ER, FloorRow);
		}
	}
	if (TotalW <= KINDA_SMALL_NUMBER)
	{
		return RowNames[FMath::RandRange(0, RowNames.Num() - 1)];
	}
	const float P = FMath::FRand() * TotalW;
	float Acc = 0.f;
	for (const FName& N : RowNames)
	{
		if (const FDFEnemyTableRow* ER = EnemyDataTable->FindRow<FDFEnemyTableRow>(N, TEXT("DFDungeon|Weight2")))
		{
			Acc += ComputeSpawnWeight(*ER, FloorRow);
			if (P <= Acc)
			{
				return N;
			}
		}
	}
	return RowNames[RowNames.Num() - 1];
}

#if !UE_BUILD_SHIPPING
void UDFDungeonManager::Dev_ForceFloorCleared()
{
	if (!IsAuthorityWorld())
	{
		return;
	}
	bFloorCleared = false;
	ClearFloorActors();
	PerformFloorCleared();
}

void UDFDungeonManager::Dev_RevealAllMinimapRooms()
{
	for (TObjectPtr<ADFMinimapRoom> R : RegisteredMinimapRooms)
	{
		if (R)
		{
			R->RevealRoom();
		}
	}
}

void UDFDungeonManager::Dev_SpawnEnemiesAt(const FName RowName, int32 const Count, AActor* const Anchor)
{
	if (!IsAuthorityWorld() || !EnemyDataTable || RowName.IsNone() || !Anchor || !GetWorld())
	{
		return;
	}
	if (const FDFEnemyTableRow* const ER = EnemyDataTable->FindRow<FDFEnemyTableRow>(RowName, TEXT("UDFDungeonManager::Dev_SpawnEnemiesAt"), false))
	{
		if (!ER->EnemyClass)
		{
			return;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		const FVector Base = Anchor->GetActorLocation();
		const int32 N = FMath::Max(1, Count);
		for (int32 i = 0; i < N; ++i)
		{
			const float Ang = (2.f * 3.14159265f * static_cast<float>(i)) / static_cast<float>(N);
			const FVector Off(FMath::Cos(Ang) * 400.f, FMath::Sin(Ang) * 400.f, 0.f);
			const FTransform T(FRotator::ZeroRotator, Base + Off);
			if (AActor* const Sp = GetWorld()->SpawnActor<AActor>(ER->EnemyClass, T, Params))
			{
				RegisterSpawnedEnemy(Sp);
				if (ADFEnemyBase* const Ch = Cast<ADFEnemyBase>(Sp))
				{
					Ch->InitializeFromDataTable(EnemyDataTable, RowName);
				}
			}
		}
		EnemiesRemaining = SpawnedEnemies.Num();
		bFloorCleared = false;
	}
}

void UDFDungeonManager::Dev_SpawnAt(const FName RowName, AActor* const Anchor)
{
	Dev_SpawnEnemiesAt(RowName, 1, Anchor);
}
#endif
