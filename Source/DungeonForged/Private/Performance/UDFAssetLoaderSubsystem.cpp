// Source/DungeonForged/Private/Performance/UDFAssetLoaderSubsystem.cpp
#include "Performance/UDFAssetLoaderSubsystem.h"
#include "ADFDungeonManager.h"
#include "Data/DFDataTableStructs.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"

void UDFAssetLoaderSubsystem::PreloadFloorAssets(const int32 FloorNumber)
{
	if (LoadedFloorNumber != INDEX_NONE && LoadedFloorNumber != FloorNumber)
	{
		UnloadPreviousFloorAssets();
	}

	FDFDungeonFloorRow Row;
	bool bHasRow = false;
	UDataTable* FloorTable = DungeonFloorTable;
	if (!FloorTable)
	{
		if (UWorld* const W = GetWorld())
		{
			if (UGameInstance* const Gi = W->GetGameInstance())
			{
				if (const UDFDungeonManager* const Dm = Gi->GetSubsystem<UDFDungeonManager>())
				{
					FloorTable = Dm->DungeonFloorTable;
				}
			}
		}
	}
	if (FloorTable)
	{
		bool bFound = false;
		FloorTable->ForeachRow<FDFDungeonFloorRow>(
			TEXT("UDFAssetLoaderSubsystem::PreloadFloorAssets"),
			[FloorNumber, &Row, &bHasRow, &bFound](const FName& /*Key*/, const FDFDungeonFloorRow& R)
			{
				if (bFound)
				{
					return;
				}
				if (R.FloorNumber == FloorNumber)
				{
					Row = R;
					bHasRow = bFound = true;
				}
			});
	}

	UDataTable* EnemyTbl = EnemyDataTable;
	if (!EnemyTbl)
	{
		if (UWorld* const W = GetWorld())
		{
			if (UGameInstance* const Gi = W->GetGameInstance())
			{
				if (const UDFDungeonManager* const Dm = Gi->GetSubsystem<UDFDungeonManager>())
				{
					EnemyTbl = Dm->EnemyDataTable;
				}
			}
		}
	}

	if (!bHasRow || !EnemyTbl)
	{
		OnFloorAssetsReady.Broadcast();
		return;
	}

	TArray<FSoftObjectPath> ToLoad;
	for (const FName& En : Row.PossibleEnemyRows)
	{
		if (const FDFEnemyTableRow* const Er = EnemyTbl->FindRow<FDFEnemyTableRow>(En, TEXT("Preload|Enemy"), false))
		{
			AddEnemyRowPaths(*Er, ToLoad);
		}
	}
	if (Row.bHasBoss && !Row.BossEnemyRow.IsNone())
	{
		if (const FDFEnemyTableRow* const Er = EnemyTbl->FindRow<FDFEnemyTableRow>(Row.BossEnemyRow, TEXT("Preload|Boss"), false))
		{
			AddEnemyRowPaths(*Er, ToLoad);
		}
	}

	if (ToLoad.Num() < 1)
	{
		OnFloorAssetsReady.Broadcast();
		return;
	}
	if (ActiveFloorLoad.IsValid())
	{
		ActiveFloorLoad->CancelHandle();
		ActiveFloorLoad.Reset();
	}
	StartAsyncLoad(
		ToLoad,
		[this, FloorNumber, LoadedPaths = ToLoad]()
		{
			ActiveFloorLoad.Reset();
			LoadedFloorNumber = FloorNumber;
			LoadedFloorPaths = LoadedPaths;
			OnFloorAssetsReady.Broadcast();
		},
		ActiveFloorLoad);
}

void UDFAssetLoaderSubsystem::UnloadPreviousFloorAssets()
{
	if (ActiveFloorLoad.IsValid())
	{
		ActiveFloorLoad->CancelHandle();
		ActiveFloorLoad.Reset();
	}
	if (LoadedFloorPaths.Num() > 0)
	{
		FStreamableManager& Sm = UAssetManager::GetStreamableManager();
		for (const FSoftObjectPath& Path : LoadedFloorPaths)
		{
			Sm.Unload(Path);
		}
		LoadedFloorPaths.Reset();
	}
	LoadedFloorNumber = INDEX_NONE;
}

void UDFAssetLoaderSubsystem::AddEnemyRowPaths(const FDFEnemyTableRow& Row, TArray<FSoftObjectPath>& OutPaths) const
{
	if (Row.EnemyClass)
	{
		OutPaths.Emplace(Row.EnemyClass);
	}
	if (Row.AIBehaviorTree)
	{
		OutPaths.Emplace(Row.AIBehaviorTree);
	}
	for (const TSoftObjectPtr<UAnimMontage>& Soft : Row.TauntMontages)
	{
		if (!Soft.IsNull())
		{
			OutPaths.Add(Soft.ToSoftObjectPath());
		}
	}
	if (!Row.SpawnBirthMontage.IsNull())
	{
		OutPaths.Add(Row.SpawnBirthMontage.ToSoftObjectPath());
	}
}

void UDFAssetLoaderSubsystem::AddAbilityRowPaths(const FDFAbilityTableRow& Row, TArray<FSoftObjectPath>& OutPaths) const
{
	if (Row.AbilityClass)
	{
		OutPaths.Emplace(Row.AbilityClass);
	}
	if (Row.Icon)
	{
		OutPaths.Emplace(Row.Icon);
	}
}

void UDFAssetLoaderSubsystem::PreloadAbilityAssets(const TArray<FName>& AbilityRowNames)
{
	if (!AbilityDataTable || AbilityRowNames.Num() < 1)
	{
		return;
	}
	TArray<FSoftObjectPath> ToLoad;
	for (const FName& Rn : AbilityRowNames)
	{
		if (const FDFAbilityTableRow* const Ar = AbilityDataTable->FindRow<FDFAbilityTableRow>(Rn, TEXT("Preload|Ability"), false))
		{
			AddAbilityRowPaths(*Ar, ToLoad);
		}
	}
	if (ToLoad.Num() < 1)
	{
		OnAbilityAssetsReady.Broadcast();
		return;
	}
	if (ActiveAbilityLoad.IsValid())
	{
		ActiveAbilityLoad->CancelHandle();
		ActiveAbilityLoad.Reset();
	}
	StartAsyncLoad(
		ToLoad,
		[this]()
		{
			ActiveAbilityLoad.Reset();
			OnAbilityAssetsReady.Broadcast();
		},
		ActiveAbilityLoad);
}

void UDFAssetLoaderSubsystem::StartAsyncLoad(
	const TArray<FSoftObjectPath>& Paths, TFunction<void()>&& OnDone, TSharedPtr<FStreamableHandle>& OutHandleSlot)
{
	if (Paths.Num() < 1)
	{
		if (OnDone)
		{
			OnDone();
		}
		return;
	}
	FStreamableManager& Sm = UAssetManager::GetStreamableManager();
	OutHandleSlot = Sm.RequestAsyncLoad(
		Paths,
		FStreamableDelegate::CreateWeakLambda(
			this,
			[Cb = MoveTemp(OnDone)]() mutable
			{
				if (Cb)
				{
					Cb();
				}
			}),
		FStreamableManager::AsyncLoadHighPriority, false, false, TEXT("UDFAssetLoaderSubsystem"));
}
