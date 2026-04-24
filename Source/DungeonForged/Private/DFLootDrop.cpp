// Source/DungeonForged/Private/DFLootDrop.cpp

#include "DFLootDrop.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/DFDataTableStructs.h"
#include "DFInventoryComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "GameFramework/PlayerState.h"

ADFLootDrop::ADFLootDrop()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bNetLoadOnClient = true;
	SetReplicateMovement(true);

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	SetRootComponent(ItemMesh);
	// Sim on an empty/placeholder mesh, or on net clients, triggers "incompatible collision" with physics.
	ItemMesh->SetSimulatePhysics(false);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ItemMesh->SetCollisionObjectType(ECC_PhysicsBody);
	ItemMesh->SetCollisionResponseToAllChannels(ECR_Block);
	ItemMesh->SetNotifyRigidBodyCollision(true);

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->SetupAttachment(ItemMesh);
	PickupSphere->InitSphereRadius(64.f);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PickupSphere->SetGenerateOverlapEvents(true);
}

void ADFLootDrop::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, Loot);
}

void ADFLootDrop::BeginPlay()
{
	Super::BeginPlay();
	PickupSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ADFLootDrop::OnPickupSphereBeginOverlap);
	if (HasAuthority() && bInitialized)
	{
		ApplyVisualsFromDataTable();
	}
}

void ADFLootDrop::InitLoot(
	UDataTable* InItemDataTable, FName InItemRowName, FVector WorldImpulse, bool bInRandomizeImpulse)
{
	Loot.ItemRowName = InItemRowName;
	if (InItemDataTable)
	{
		ItemDataTable = InItemDataTable;
		Loot.ItemDataTablePath = FSoftObjectPath(InItemDataTable);
	}
	bInitialized = true;

	if (HasAuthority())
	{
		ApplyVisualsFromDataTable();
		if (ItemMesh)
		{
			FVector Imp = WorldImpulse;
			if (bInRandomizeImpulse)
			{
				const FVector R = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(0.2f, 1.f))
					.GetSafeNormal();
				Imp = R * FMath::FRandRange(DropImpulseMin, DropImpulseMax);
			}
			ItemMesh->AddImpulse(Imp, NAME_None, true);
		}
	}
}

void ADFLootDrop::OnRep_Loot()
{
	ApplyVisualsFromDataTable();
}

void ADFLootDrop::ApplyVisualsFromDataTable()
{
	if (ItemDataTable == nullptr && !Loot.ItemDataTablePath.IsNull())
	{
		ItemDataTable = Cast<UDataTable>(Loot.ItemDataTablePath.TryLoad());
	}
	if (!ItemDataTable || Loot.ItemRowName.IsNone())
	{
		return;
	}
	const FDFItemTableRow* const Row = ItemDataTable->FindRow<FDFItemTableRow>(Loot.ItemRowName, TEXT("DFLoot|ApplyVisuals"));
	if (!Row)
	{
		return;
	}
	if (ItemMesh)
	{
		if (UStaticMesh* M = Row->ItemMesh.Get())
		{
			// Reapply collision and physics after mesh change (asset can reset or lack simple collision for sim).
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetStaticMesh(M);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			ItemMesh->SetCollisionObjectType(ECC_PhysicsBody);
			ItemMesh->SetCollisionResponseToAllChannels(ECR_Block);
			ItemMesh->SetNotifyRigidBodyCollision(true);
			ItemMesh->RecreatePhysicsState();
			// Server simulates; clients follow replicated root transform (bReplicateMovement).
			if (HasAuthority())
			{
				ItemMesh->SetSimulatePhysics(true);
			}
		}
		ApplyRarityEmissive();
	}
}

FLinearColor ADFLootDrop::RarityToLinearColor(EItemRarity Rarity)
{
	switch (Rarity)
	{
		case EItemRarity::Uncommon: return FLinearColor(0.2f, 0.9f, 0.2f, 1.f);
		case EItemRarity::Rare: return FLinearColor(0.2f, 0.45f, 1.f, 1.f);
		case EItemRarity::Epic: return FLinearColor(0.75f, 0.2f, 0.95f, 1.f);
		case EItemRarity::Legendary: return FLinearColor(1.f, 0.55f, 0.1f, 1.f);
		case EItemRarity::Common:
		default: return FLinearColor(0.5f, 0.5f, 0.52f, 1.f);
	}
}

void ADFLootDrop::ApplyRarityEmissive()
{
	if (!ItemDataTable || Loot.ItemRowName.IsNone())
	{
		return;
	}
	const FDFItemTableRow* const Row = ItemDataTable->FindRow<FDFItemTableRow>(Loot.ItemRowName, TEXT("DFLoot|RarityColor"));
	if (!Row || !ItemMesh)
	{
		return;
	}
	const FLinearColor C = RarityToLinearColor(Row->Rarity);
	if (RarityBaseMaterial)
	{
		RarityMaterialMID = UMaterialInstanceDynamic::Create(RarityBaseMaterial, this);
		if (RarityMaterialMID)
		{
			ItemMesh->SetMaterial(0, RarityMaterialMID);
		}
	}
	else
	{
		RarityMaterialMID = ItemMesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (RarityMaterialMID)
	{
		RarityMaterialMID->SetVectorParameterValue(RarityColorParameterName, C);
	}
}

void ADFLootDrop::OnPickupSphereBeginOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	if (!OtherActor || Loot.ItemRowName.IsNone())
	{
		return;
	}
	APawn* const P = Cast<APawn>(OtherActor);
	if (!P)
	{
		return;
	}
	UDFInventoryComponent* Inv = P->FindComponentByClass<UDFInventoryComponent>();
	if (!Inv && P->GetPlayerState())
	{
		Inv = P->GetPlayerState()->FindComponentByClass<UDFInventoryComponent>();
	}
	if (!Inv)
	{
		return;
	}
	if (HasAuthority())
	{
		if (Inv->AddItem(Loot.ItemRowName, 1))
		{
			OnPickedUp.Broadcast(P, Loot.ItemRowName);
			PlayPickupLocalFeedback();
			Multicast_PlayPickupVFX(P, GetActorLocation());
			Destroy();
		}
	}
	else if (P->IsLocallyControlled())
	{
		Inv->ServerPickUpFromLoot(this);
	}
}

void ADFLootDrop::PlayPickupLocalFeedback() {}

void ADFLootDrop::Multicast_PlayPickupVFX_Implementation(AActor* InteractingActor, FVector VFXLocation)
{
	if (PickupNiagaraVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, PickupNiagaraVFX, VFXLocation, FRotator::ZeroRotator);
	}
	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, VFXLocation);
	}
	(void)InteractingActor;
}
