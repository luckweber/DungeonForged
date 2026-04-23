// Source/DungeonForged/Private/Interaction/ADFChest.cpp
#include "Interaction/ADFChest.h"
#include "Components/SkeletalMeshComponent.h"
#include "DFLootGeneratorSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Animation/AnimInstance.h"
#include "Particles/ParticleSystem.h"
#include "GameFramework/Character.h"

ADFChest::ADFChest()
{
	ChestSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ChestSkeletalMesh"));
	ChestSkeletalMesh->SetupAttachment(RootComponent);
	bSingleUse = true;
}

void ADFChest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADFChest, bIsOpen);
}

FText ADFChest::GetInteractionText_Implementation() const
{
	if (bIsOpen)
	{
		return NSLOCTEXT("DF", "ChestOpen", "Empty");
	}
	return NSLOCTEXT("DF", "ChestInteract", "Open chest");
}

bool ADFChest::CanInteract_Implementation(ACharacter* Interactor) const
{
	return bIsInteractable && !bIsOpen && Interactor;
}

void ADFChest::OnRep_IsOpen()
{
	if (bIsOpen)
	{
		bIsInteractable = false;
	}
}

void ADFChest::Interact_Implementation(ACharacter* Interactor)
{
	if (!HasAuthority() || !Interactor)
	{
		return;
	}
	if (bIsOpen)
	{
		return;
	}
	if (USkeletalMeshComponent* const Skel = ChestSkeletalMesh)
	{
		if (UAnimInstance* const AI = Skel->GetAnimInstance())
		{
			if (OpenMontage)
			{
				AI->Montage_Play(OpenMontage, 1.f);
			}
		}
	}
	if (OpenVFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			this, OpenVFX, GetActorLocation() + FVector(0.f, 0.f, 40.f), FRotator::ZeroRotator, FVector(1.f), true);
	}
	if (UWorld* const W = GetWorld())
	{
		if (UDFLootGeneratorSubsystem* const Loot = W->GetSubsystem<UDFLootGeneratorSubsystem>())
		{
			if (LootPoolDataTable && !LootTableRow.IsNone() && Loot->ItemDataTable)
			{
				Loot->RollGuaranteedDropsFromPool(
					LootPoolDataTable, LootTableRow, Loot->ItemDataTable, MinRarity, MinDropCount, MaxDropCount, GetActorLocation() + FVector(0.f, 0.f, 20.f), FVector(0.f, 0.f, 180.f));
			}
		}
	}
	bIsOpen = true;
	bIsInteractable = false;
}
