// Source/DungeonForged/Private/UI/Minimap/ADFMinimapRoom.cpp
#include "UI/Minimap/ADFMinimapRoom.h"
#include "ADFDungeonManager.h"
#include "Components/BoxComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"

namespace
{
	/** Sufficient for typical humanoid + vertical props inside the room. */
	constexpr float MinimapRoomZExtentUU = 400.f;
}

ADFMinimapRoom::ADFMinimapRoom()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	RoomBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("RoomBounds"));
	SetRootComponent(RoomBounds);
	RoomBounds->SetBoxExtent(FVector(1000.f, 1000.f, MinimapRoomZExtentUU));
	RoomBounds->SetGenerateOverlapEvents(true);
	RoomBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RoomBounds->SetCollisionObjectType(ECC_WorldStatic);
	RoomBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	RoomBounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ADFMinimapRoom::BeginPlay()
{
	Super::BeginPlay();

	// If designer left default extent, size from RoomSize.
	if (FMath::IsNearlyEqual(RoomBounds->GetUnscaledBoxExtent().X, 1000.f)
		&& (RoomSize.X > 1.f || RoomSize.Y > 1.f))
	{
		RoomBounds->SetBoxExtent(FVector(RoomSize.X * 0.5f, RoomSize.Y * 0.5f, MinimapRoomZExtentUU));
	}
	if (RoomCenter.IsZero())
	{
		RoomCenter = GetActorLocation();
	}
	else
	{
		SetActorLocation(RoomCenter);
	}

	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFDungeonManager* const DM = GI->GetSubsystem<UDFDungeonManager>())
		{
			DM->RegisterMinimapRoom(this);
		}
	}
}

void ADFMinimapRoom::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFDungeonManager* const DM = GI->GetSubsystem<UDFDungeonManager>())
		{
			DM->UnregisterMinimapRoom(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ADFMinimapRoom::RevealRoom()
{
	if (bIsRevealed)
	{
		return;
	}
	bIsRevealed = true;

	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFDungeonManager* const DM = GI->GetSubsystem<UDFDungeonManager>())
		{
			DM->NotifyMinimapRoomRevealed(this);
		}
	}
}

void ADFMinimapRoom::VisitRoom()
{
	if (bIsVisited)
	{
		return;
	}
	bIsVisited = true;

	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFDungeonManager* const DM = GI->GetSubsystem<UDFDungeonManager>())
		{
			DM->NotifyMinimapRoomVisited(this);
		}
	}
}

UTexture2D* ADFMinimapRoom::GetIconTexture_Implementation() const
{
	return IconOverride;
}

bool ADFMinimapRoom::IsAdjacentToRevealed() const
{
	if (bIsRevealed)
	{
		return false;
	}
	for (const TObjectPtr<ADFMinimapRoom>& N : NeighborRooms)
	{
		if (N && N->bIsRevealed)
		{
			return true;
		}
	}
	return false;
}
