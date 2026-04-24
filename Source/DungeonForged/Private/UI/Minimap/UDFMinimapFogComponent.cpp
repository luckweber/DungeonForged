// Source/DungeonForged/Private/UI/Minimap/UDFMinimapFogComponent.cpp
#include "UI/Minimap/UDFMinimapFogComponent.h"
#include "ADFDungeonManager.h"
#include "UI/Minimap/ADFMinimapRoom.h"
#include "Components/SphereComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/UObjectGlobals.h"

UDFMinimapFogComponent::UDFMinimapFogComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDFMinimapFogComponent::EnsureOverlapSphere()
{
	if (OverlapSphere)
	{
		return;
	}

	AActor* const Owner = GetOwner();
	UWorld* const World = GetWorld();
	if (!Owner || Owner->HasAnyFlags(RF_ClassDefaultObject) || !World || !World->IsGameWorld())
	{
		return;
	}

	const FName SphereName = MakeUniqueObjectName(this, USphereComponent::StaticClass(), TEXT("MinimapFogOverlap"));
	OverlapSphere = NewObject<USphereComponent>(this, USphereComponent::StaticClass(), SphereName, RF_Transactional);
	if (!OverlapSphere)
	{
		return;
	}

	OverlapSphere->SetupAttachment(this);
	OverlapSphere->SetSphereRadius(SphereRadius);
	OverlapSphere->SetGenerateOverlapEvents(true);
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapSphere->SetCollisionObjectType(ECC_Pawn);
	OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);

	if (!OverlapSphere->IsRegistered())
	{
		OverlapSphere->RegisterComponent();
	}
}

void UDFMinimapFogComponent::BeginPlay()
{
	Super::BeginPlay();

	EnsureOverlapSphere();
	if (!OverlapSphere)
	{
		return;
	}

	OverlapSphere->SetSphereRadius(SphereRadius);
	OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &UDFMinimapFogComponent::OnRoomBoundaryOverlap);
	OverlapSphere->OnComponentEndOverlap.AddDynamic(this, &UDFMinimapFogComponent::OnRoomBoundaryEndOverlap);
}

void UDFMinimapFogComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (OverlapSphere)
	{
		OverlapSphere->OnComponentBeginOverlap.RemoveAll(this);
		OverlapSphere->OnComponentEndOverlap.RemoveAll(this);
		OverlapSphere->DestroyComponent();
		OverlapSphere = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void UDFMinimapFogComponent::OnRoomBoundaryOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* const OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	ADFMinimapRoom* const R = OtherActor ? Cast<ADFMinimapRoom>(OtherActor) : nullptr;
	if (!R)
	{
		return;
	}
	R->RevealRoom();

	UGameInstance* GI = nullptr;
	if (AActor* const O = GetOwner())
	{
		GI = O->GetGameInstance();
	}
	if (!GI && GetWorld())
	{
		GI = GetWorld()->GetGameInstance();
	}
	if (UDFDungeonManager* const DM = GI ? GI->GetSubsystem<UDFDungeonManager>() : nullptr)
	{
		DM->SetPlayerCurrentMinimapRoom(R);
	}
}

void UDFMinimapFogComponent::OnRoomBoundaryEndOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* const OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/)
{
	ADFMinimapRoom* const R = OtherActor ? Cast<ADFMinimapRoom>(OtherActor) : nullptr;
	if (!R)
	{
		return;
	}
	R->VisitRoom();

	UGameInstance* GI = nullptr;
	if (AActor* const O = GetOwner())
	{
		GI = O->GetGameInstance();
	}
	if (!GI && GetWorld())
	{
		GI = GetWorld()->GetGameInstance();
	}
	if (UDFDungeonManager* const DM = GI ? GI->GetSubsystem<UDFDungeonManager>() : nullptr)
	{
		if (DM->GetPlayerCurrentMinimapRoom() == R)
		{
			DM->SetPlayerCurrentMinimapRoom(nullptr);
		}
	}
}
