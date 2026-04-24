// Source/DungeonForged/Private/ADFDungeonRunGameMode.cpp

#include "ADFDungeonRunGameMode.h"
#include "ADFDungeonManager.h"
#include "Engine/GameInstance.h"

ADFDungeonRunGameMode::ADFDungeonRunGameMode()
	: bAutoStartOnBeginPlay(true)
	, StartFloorNumber(1)
{
}

void ADFDungeonRunGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (!bAutoStartOnBeginPlay)
	{
		return;
	}
	if (UWorld* const W = GetWorld())
	{
		if (W->GetNetMode() == NM_Client)
		{
			return;
		}
	}
	StartDungeonRun();
}

void ADFDungeonRunGameMode::StartDungeonRun()
{
	if (UWorld* const W = GetWorld())
	{
		if (W->GetNetMode() == NM_Client)
		{
			return;
		}
	}

	UGameInstance* const GI = GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("ADFDungeonRunGameMode: no GameInstance"));
		return;
	}

	UDFDungeonManager* const DM = GI->GetSubsystem<UDFDungeonManager>();
	if (!DM)
	{
		UE_LOG(LogTemp, Error, TEXT("ADFDungeonRunGameMode: UDFDungeonManager subsystem missing"));
		return;
	}

	if (!EnemyDataTable || !DungeonFloorTable)
	{
		UE_LOG(
			LogTemp, Error,
			TEXT("ADFDungeonRunGameMode: set EnemyDataTable and DungeonFloorTable (Game Mode defaults or Blueprint)"));
		return;
	}

	DM->EnemyDataTable = EnemyDataTable;
	DM->DungeonFloorTable = DungeonFloorTable;
	DM->PCGOwnerActor = PCGOwnerActor;
	DM->SpawnPointsPreview = SpawnPointsPreview;
	DM->StartFloor(StartFloorNumber);
}
