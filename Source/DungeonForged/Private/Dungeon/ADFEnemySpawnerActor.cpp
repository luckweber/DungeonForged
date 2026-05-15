// Source/DungeonForged/Private/Dungeon/ADFEnemySpawnerActor.cpp
#include "Dungeon/ADFEnemySpawnerActor.h"
#include "ADFDungeonManager.h"
#include "Characters/ADFEnemyBase.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Data/DFDataTableStructs.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Run/DFRunManager.h"
#include "TimerManager.h"

namespace
{
	float SumSpawnWeights(TArray<FDFWeightedEnemySpawnEntry> const& Entries, UDataTable const* const Table)
	{
		float S = 0.f;
		for (FDFWeightedEnemySpawnEntry const& E : Entries)
		{
			if (E.EnemyRowName.IsNone() || !Table)
			{
				continue;
			}
			if (FDFEnemyTableRow const* const Row =
					Table->FindRow<FDFEnemyTableRow>(E.EnemyRowName, TEXT("ADFEnemySpawnerActor|Weights"), false))
			{
				if (Row->EnemyClass)
				{
					S += FMath::Max(0.0001f, E.Weight);
				}
			}
		}
		return S;
	}
}

ADFEnemySpawnerActor::ADFEnemySpawnerActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	EditorBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("EditorBounds"));
	EditorBounds->SetupAttachment(SceneRoot);
	EditorBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EditorBounds->SetCanEverAffectNavigation(false);
	EditorBounds->SetHiddenInGame(true);
	EditorBounds->SetBoxExtent(FVector(500.f, 500.f, 100.f));
}

void ADFEnemySpawnerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (EditorBounds)
	{
		EditorBounds->SetBoxExtent(SpawnHalfExtents);
	}
}

void ADFEnemySpawnerActor::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority())
	{
		return;
	}
	if (!bSpawnOnBeginPlay)
	{
		return;
	}
	if (SpawnDelaySeconds <= 0.f)
	{
		TriggerSpawnWave();
		return;
	}
	GetWorldTimerManager().SetTimer(SpawnDelayTimerHandle, this, &ADFEnemySpawnerActor::HandleSpawnDelayElapsed, SpawnDelaySeconds, false);
}

void ADFEnemySpawnerActor::HandleSpawnDelayElapsed()
{
	TriggerSpawnWave();
}

void ADFEnemySpawnerActor::TriggerSpawnWave()
{
	if (!HasAuthority() || !GetWorld())
	{
		return;
	}

	UGameInstance* const GI = GetWorld()->GetGameInstance();
	if (!GI)
	{
		return;
	}

	UDFDungeonManager* const DM = GI->GetSubsystem<UDFDungeonManager>();

	if (bRequireMatchingRunFloor)
	{
		int32 ActiveFloor = INDEX_NONE;
		if (DM && DM->CurrentFloor > 0)
		{
			ActiveFloor = DM->CurrentFloor;
		}
		else if (UDFRunManager const* const RM = GI->GetSubsystem<UDFRunManager>())
		{
			ActiveFloor = RM->GetCurrentRunState().CurrentFloor;
		}
		if (ActiveFloor != RequiredRunFloor)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("ADFEnemySpawnerActor '%s': RequireMatchingRunFloor blocked spawn — Required=%d, active=%d (set Required = boss floor or disable gate)."),
				*GetNameSafe(this), RequiredRunFloor, ActiveFloor);
			return;
		}
	}

	UDataTable* const Table = EnemyDataTableOverride ? EnemyDataTableOverride.Get() : (DM ? DM->EnemyDataTable.Get() : nullptr);
	if (!Table || WeightedEnemyEntries.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ADFEnemySpawnerActor '%s': need WeightedEnemyEntries and EnemyDataTable (Game Mode / DungeonManager or Override)."),
			*GetNameSafe(this));
		return;
	}

	const int32 WaveCount = ComputeSpawnCount();
	if (WaveCount <= 0)
	{
		return;
	}

	const float WeightSum = SumSpawnWeights(WeightedEnemyEntries, Table);
	if (WeightSum <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogTemp, Warning, TEXT("ADFEnemySpawnerActor '%s': no valid weighted rows (missing class in DT?)."), *GetNameSafe(this));
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (int32 i = 0; i < WaveCount; ++i)
	{
		FName const RowName = PickWeightedEnemyRow(Table);
		if (RowName.IsNone())
		{
			continue;
		}
		FDFEnemyTableRow const* const ER = Table->FindRow<FDFEnemyTableRow>(RowName, TEXT("ADFEnemySpawnerActor|Spawn"), false);
		if (!ER || !ER->EnemyClass)
		{
			continue;
		}

		FVector Loc = ComputeRandomSpawnLocation();
		FRotator Rot = GetActorRotation();
		if (bRandomYaw)
		{
			Rot.Yaw += FMath::FRandRange(0.f, 360.f);
		}
		FTransform SpawnTm(Rot, Loc);
		if (DM)
		{
			FVector NavExt = FVector::ZeroVector;
			if (bSnapSpawnToNavMesh && EditorBounds)
			{
				FVector const E = EditorBounds->GetScaledBoxExtent();
				NavExt =
					FVector(FMath::Max(E.X, 50.f), FMath::Max(E.Y, 50.f), FMath::Max(NavMeshVerticalQueryCm, 100.f));
			}
			DM->SnapEnemySpawnToWorld(SpawnTm, ER->EnemyClass, NavExt, bSnapSpawnToNavMesh);
			if (!IsWorldPointInsideBoundsFootprint(SpawnTm.GetLocation()))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("ADFEnemySpawnerActor '%s': snapped spawn XY outside EditorBounds footprint (check volume / slopes). SnapLoc=%s"),
					*GetNameSafe(this), *SpawnTm.GetLocation().ToString());
			}
		}

		if (AActor* const Spawned = GetWorld()->SpawnActor<AActor>(ER->EnemyClass, SpawnTm, Params))
		{
			if (DM && bRegisterWithDungeonManager)
			{
				DM->RegisterSpawnedEnemy(Spawned);
			}
			if (ADFEnemyBase* const Char = Cast<ADFEnemyBase>(Spawned))
			{
				Char->InitializeFromDataTable(Table, RowName);
			}
		}
	}
}

int32 ADFEnemySpawnerActor::ComputeSpawnCount() const
{
	int32 Lo = FMath::Min(MinSpawnCount, MaxSpawnCount);
	int32 Hi = FMath::Max(MinSpawnCount, MaxSpawnCount);
	if (Hi <= 0)
	{
		return 0;
	}
	return (Lo == Hi) ? Lo : FMath::RandRange(Lo, Hi);
}

FName ADFEnemySpawnerActor::PickWeightedEnemyRow(UDataTable const* const EnemyTable) const
{
	float const Total = SumSpawnWeights(WeightedEnemyEntries, EnemyTable);
	if (Total <= KINDA_SMALL_NUMBER)
	{
		return NAME_None;
	}
	float Pick = FMath::FRand() * Total;
	for (FDFWeightedEnemySpawnEntry const& E : WeightedEnemyEntries)
	{
		if (E.EnemyRowName.IsNone())
		{
			continue;
		}
		if (FDFEnemyTableRow const* const Row =
				EnemyTable->FindRow<FDFEnemyTableRow>(E.EnemyRowName, TEXT("ADFEnemySpawnerActor|Pick"), false))
		{
			if (!Row->EnemyClass)
			{
				continue;
			}
			float const W = FMath::Max(0.0001f, E.Weight);
			Pick -= W;
			if (Pick <= 0.f)
			{
				return E.EnemyRowName;
			}
		}
	}
	return NAME_None;
}

FVector ADFEnemySpawnerActor::ComputeRandomSpawnLocation() const
{
	FTransform const Tm = GetActorTransform();
	FVector const Origin = Tm.GetLocation();

	switch (Distribution)
	{
	case EDFEnemySpawnerDistribution::ActorOrigin:
		return Origin;

	case EDFEnemySpawnerDistribution::SphereVolume:
	{
		if (bProjectToFloor)
		{
			// Horizontal disk at top of sphere AABB — downward snap finds floor under the volume.
			float const rDisk = SphereRadiusCm * FMath::Sqrt(FMath::FRand());
			float const Ang = FMath::FRandRange(0.f, 2.f * UE_PI);
			return Tm.TransformPosition(FVector(rDisk * FMath::Cos(Ang), rDisk * FMath::Sin(Ang), SphereRadiusCm));
		}
		float const U = FMath::FRand();
		float const R = SphereRadiusCm * FMath::Pow(U, 1.f / 3.f);
		if (bSpawnOnVolumeBottomFace)
		{
			// Lower half of oriented unit sphere (local +Z up): keeps samples below the horizon of the pivot.
			FVector LocalDir;
			int32 Attempts = 0;
			do
			{
				LocalDir = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f));
				++Attempts;
			} while (Attempts < 64 && (LocalDir.SizeSquared() > 1.f || LocalDir.Z > 0.f));
			if (Attempts >= 64)
			{
				LocalDir = FVector(0.f, 0.f, -1.f);
			}
			else
			{
				LocalDir.Normalize();
			}
			return Tm.TransformPosition(LocalDir * R);
		}
		FVector const Dir = FMath::VRand().GetSafeNormal();
		return Origin + Dir * R;
	}

	case EDFEnemySpawnerDistribution::RingXY:
	{
		float const Inner = FMath::Min(RingInnerRadiusCm, RingOuterRadiusCm);
		float const Outer = FMath::Max(RingInnerRadiusCm, RingOuterRadiusCm);
		float const Inner2 = Inner * Inner;
		float const Outer2 = Outer * Outer;
		float const t = FMath::FRand();
		float const Radius = FMath::Sqrt(FMath::Lerp(Inner2, Outer2, t));
		float const Ang = FMath::FRandRange(0.f, 2.f * UE_PI);
		float ZLocal = 0.f;
		if (bProjectToFloor)
		{
			ZLocal = SpawnHalfExtents.Z;
		}
		else if (bSpawnOnVolumeBottomFace)
		{
			ZLocal = -SpawnHalfExtents.Z;
		}
		FVector const Local(Radius * FMath::Cos(Ang), Radius * FMath::Sin(Ang), ZLocal);
		return Tm.TransformPosition(Local);
	}

	case EDFEnemySpawnerDistribution::OrientedBox:
	default:
	{
		float ZLocal;
		if (bProjectToFloor)
		{
			ZLocal = SpawnHalfExtents.Z;
		}
		else if (bSpawnOnVolumeBottomFace)
		{
			ZLocal = -SpawnHalfExtents.Z;
		}
		else
		{
			ZLocal = FMath::FRandRange(-SpawnHalfExtents.Z, SpawnHalfExtents.Z);
		}
		FVector const Local(
			FMath::FRandRange(-SpawnHalfExtents.X, SpawnHalfExtents.X),
			FMath::FRandRange(-SpawnHalfExtents.Y, SpawnHalfExtents.Y),
			ZLocal);
		return Tm.TransformPosition(Local);
	}
	}
}

bool ADFEnemySpawnerActor::IsWorldPointInsideBoundsFootprint(FVector const& WorldPosition) const
{
	if (!EditorBounds)
	{
		return true;
	}
	FVector const Local = EditorBounds->GetComponentTransform().InverseTransformPosition(WorldPosition);
	FVector const Ext = EditorBounds->GetScaledBoxExtent();
	float const Tol = 5.f;
	return FMath::Abs(Local.X) <= Ext.X + Tol && FMath::Abs(Local.Y) <= Ext.Y + Tol;
}
