// Source/DungeonForged/Private/GameModes/Nexus/ADFNexusNPCBase.cpp
#include "GameModes/Nexus/ADFNexusNPCBase.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Run/DFSaveGame.h"
#include "Net/UnrealNetwork.h"
#include "GameModes/Nexus/DFNexusUnlockData.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Interaction/UDFInteractionComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/KismetMathLibrary.h"

ADFNexusNPCBase::ADFNexusNPCBase()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionRange"));
	InteractionRange->SetSphereRadius(200.f);
	InteractionRange->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	InteractionRange->SetGenerateOverlapEvents(true);
	InteractionRange->SetCanEverAffectNavigation(false);
	InteractionRange->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
}

void ADFNexusNPCBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADFNexusNPCBase, bIsUnlocked);
}

void ADFNexusNPCBase::SetNexusUnlockedFromSave(const bool bUnlocked)
{
	bIsUnlocked = bUnlocked;
	ApplyUnlockVisual();
}

void ADFNexusNPCBase::OnRep_NexusUnlocked()
{
	ApplyUnlockVisual();
}

void ADFNexusNPCBase::BeginPlay()
{
	if (UWorld* const W = GetWorld())
	{
		if (W->GetNetMode() != NM_Client)
		{
			if (UDFSaveGame* const S = UDFSaveGame::Load())
			{
				if (S->UnlockedNPCs.Contains(NPCId))
				{
					bIsUnlocked = true;
				}
				else if (bDefaultUnlocked && UnlockConditionRow.IsNone())
				{
					bIsUnlocked = true;
				}
				else
				{
					bIsUnlocked = CheckUnlockCondition(S) || bDefaultUnlocked;
				}
			}
		}
	}
	if (GetCapsuleComponent())
	{
		InteractionRange->SetupAttachment(GetCapsuleComponent());
	}
	InteractionRange->OnComponentBeginOverlap.AddDynamic(this, &ADFNexusNPCBase::OnNexusRangeBegin);
	InteractionRange->OnComponentEndOverlap.AddDynamic(this, &ADFNexusNPCBase::OnNexusRangeEnd);
	Super::BeginPlay();
	ApplyUnlockVisual();
}

void ADFNexusNPCBase::OnNexusRangeBegin(
	UPrimitiveComponent* const /*OverlappedComponent*/,
	AActor* const OtherActor,
	UPrimitiveComponent* const /*OtherComp*/,
	int32 const /*OtherBodyIndex*/,
	bool const /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	ACharacter* const C = Cast<ACharacter>(OtherActor);
	if (!C)
	{
		return;
	}
	if (UDFInteractionComponent* const IC = C->FindComponentByClass<UDFInteractionComponent>())
	{
		IC->RegisterInteractable(this);
	}
}

void ADFNexusNPCBase::OnNexusRangeEnd(
	UPrimitiveComponent* const /*OverlappedComponent*/, AActor* const OtherActor, UPrimitiveComponent* const /*OtherComp*/, int32 const /*OtherBodyIndex*/)
{
	ACharacter* const C = Cast<ACharacter>(OtherActor);
	if (!C)
	{
		return;
	}
	if (UDFInteractionComponent* const IC = C->FindComponentByClass<UDFInteractionComponent>())
	{
		IC->UnregisterInteractable(this);
	}
}

FText ADFNexusNPCBase::GetInteractionText_Implementation() const
{
	return NSLOCTEXT("DFNexus", "NPCTalk", "Talk");
}

bool ADFNexusNPCBase::CanInteract_Implementation(ACharacter* const /*Interactor*/) const
{
	return bIsUnlocked;
}

void ADFNexusNPCBase::Interact_Implementation(ACharacter* const Interactor)
{
	if (!bIsUnlocked)
	{
		return;
	}
	if (Interactor)
	{
		const FRotator Yaw(0.f, UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Interactor->GetActorLocation()).Yaw, 0.f);
		SetActorRotation(Yaw);
	}
	if (USkeletalMeshComponent* const M = GetMesh())
	{
		if (UAnimInstance* const A = M->GetAnimInstance())
		{
			if (TalkMontage)
			{
				A->Montage_Play(TalkMontage, 1.f, EMontagePlayReturnType::MontageLength, 0.f, true);
			}
		}
	}
	if (UWorld* const W = GetWorld())
	{
		if (APlayerController* const PC = UGameplayStatics::GetPlayerController(W, 0))
		{
			if (ServiceWidgetClass)
			{
				ActiveServiceWidget = CreateWidget<UUserWidget>(PC, ServiceWidgetClass);
				if (ActiveServiceWidget)
				{
					ActiveServiceWidget->AddToViewport(50);
				}
			}
		}
	}
}

bool ADFNexusNPCBase::CheckUnlockCondition(UDFSaveGame* const Save) const
{
	if (!Save)
	{
		return bDefaultUnlocked;
	}
	if (UnlockConditionRow.IsNone())
	{
		return bDefaultUnlocked;
	}
	if (!UnlockConditionTable)
	{
		return false;
	}
	if (const FDFNexusUnlockConditionRow* const Row = UnlockConditionTable->FindRow<FDFNexusUnlockConditionRow>(UnlockConditionRow, TEXT("NexusCheck")))
	{
		return Save->TotalRuns >= Row->MinRunsCompleted
			&& Save->TotalWins >= Row->MinWinsCompleted
			&& Save->MetaLevel >= Row->MinMetaLevel;
	}
	return false;
}

void ADFNexusNPCBase::ApplyUnlockVisual()
{
	if (!bIsUnlocked)
	{
		SetActorHiddenInGame(true);
		if (USceneComponent* const R = GetRootComponent())
		{
			R->SetVisibility(false, true);
		}
		SetActorEnableCollision(false);
	}
	else
	{
		SetActorHiddenInGame(false);
		if (USceneComponent* const R = GetRootComponent())
		{
			R->SetVisibility(true, true);
		}
		SetActorEnableCollision(true);
	}
}
