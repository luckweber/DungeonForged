// Source/DungeonForged/Private/World/UDFWorldTransitionSubsystem.cpp
#include "World/UDFWorldTransitionSubsystem.h"
#include "ADFDungeonManager.h"
#include "Network/UDFGameInstance.h"
#include "Run/DFRunManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UDFWorldTransitionSubsystem::TravelToNexus(const ERunNexusTravelReason /*Reason*/)
{
	UGameInstance* const GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	UWorld* const W = GI->GetWorld();
	if (!W || W->GetNetMode() == NM_Client)
	{
		return;
	}

	FString URL = NexusMapPath;
	if (URL.IsEmpty())
	{
		if (const UDFGameInstance* const DFGI = Cast<UDFGameInstance>(GI))
		{
			URL = DFGI->MainMenuMapName;
		}
	}
	if (URL.IsEmpty())
	{
		return;
	}
	W->ServerTravel(URL, false);
}

void UDFWorldTransitionSubsystem::TravelToNextFloor(const int32 NextFloor)
{
	UGameInstance* const GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	UWorld* const W = GI->GetWorld();
	if (!W || W->GetNetMode() == NM_Client)
	{
		return;
	}

	if (UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
	{
		RM->AdvanceFloor(NextFloor - RM->GetCurrentRunState().CurrentFloor);
	}
	if (UDFDungeonManager* const DM = GI->GetSubsystem<UDFDungeonManager>())
	{
		if (DungeonRunMapPath.IsEmpty())
		{
			DM->StartFloor(NextFloor);
		}
		else
		{
			if (UWorld* const AuthWorld = GetGameInstance()->GetWorld())
			{
				const FString WithOpt = FString::Printf(
					TEXT("%s?df_run_floor=%d"),
					*DungeonRunMapPath,
					NextFloor);
				AuthWorld->ServerTravel(WithOpt, false);
			}
		}
	}
}
