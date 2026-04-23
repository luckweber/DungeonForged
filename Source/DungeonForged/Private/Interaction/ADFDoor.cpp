// Source/DungeonForged/Private/Interaction/ADFDoor.cpp
#include "Curves/CurveFloat.h"
#include "Interaction/ADFDoor.h"
#include "Interaction/UDFInteractionEventBus.h"
#include "Components/TimelineComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

ADFDoor::ADFDoor()
{
	OpenTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("OpenTimeline"));
	bSingleUse = false;
}

void ADFDoor::BeginPlay()
{
	Super::BeginPlay();
	if (Mesh)
	{
		ClosedMeshYaw = Mesh->GetRelativeRotation().Yaw;
	}
	if (UWorld* const W = GetWorld())
	{
		if (UDFInteractionEventBus* const Bus = W->GetSubsystem<UDFInteractionEventBus>())
		{
			Bus->OnGameplayEvent.AddUObject(this, &ADFDoor::OnWorldGameplayEvent);
		}
	}
	UCurveFloat* CurveToUse = OpenCurve;
	if (!CurveToUse)
	{
		UCurveFloat* const Linear = NewObject<UCurveFloat>(this, TEXT("DoorOpenLinear"));
		Linear->FloatCurve.AddKey(0.f, 0.f);
		Linear->FloatCurve.AddKey(1.f, 1.f);
		CurveToUse = Linear;
	}
	FOnTimelineFloatStatic OpenCallback;
	OpenCallback.BindUObject(this, &ADFDoor::OnOpenTimelineUpdate);
	OpenTimeline->AddInterpFloat(CurveToUse, OpenCallback);
	FOnTimelineEventStatic FinishEvent;
	FinishEvent.BindUObject(this, &ADFDoor::OnOpenTimelineFinished);
	OpenTimeline->SetTimelineFinishedFunc(FinishEvent);
	OpenTimeline->SetPlaybackPosition(0.f, false);
	OpenTimeline->SetLooping(false);
	OpenTimeline->SetTimelineLength(OpenDuration > KINDA_SMALL_NUMBER ? OpenDuration : 1.2f);
}

void ADFDoor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* const W = GetWorld())
	{
		if (UDFInteractionEventBus* const Bus = W->GetSubsystem<UDFInteractionEventBus>())
		{
			Bus->OnGameplayEvent.RemoveAll(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ADFDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADFDoor, bIsLocked);
	DOREPLIFETIME(ADFDoor, bIsOpen);
}

FText ADFDoor::GetInteractionText_Implementation() const
{
	if (bIsOpen)
	{
		return NSLOCTEXT("DF", "DoorOpen", "Open");
	}
	if (bIsLocked)
	{
		return NSLOCTEXT("DF", "DoorLocked", "Locked");
	}
	return NSLOCTEXT("DF", "DoorUse", "Open door");
}

void ADFDoor::Interact_Implementation(ACharacter* /*Interactor*/)
{
	if (!HasAuthority())
	{
		return;
	}
	if (bIsOpen)
	{
		return;
	}
	if (bIsLocked)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, TEXT("Locked"));
		}
		return;
	}
	OpenDoor();
}

void ADFDoor::LockDoor()
{
	if (!HasAuthority())
	{
		return;
	}
	bIsLocked = true;
}

void ADFDoor::UnlockDoor()
{
	if (!HasAuthority())
	{
		return;
	}
	bIsLocked = false;
}

void ADFDoor::OpenDoor()
{
	if (!HasAuthority() || bIsOpen)
	{
		return;
	}
	bIsOpen = true;
	Multicast_PlayDoorOpen();
}

void ADFDoor::Multicast_PlayDoorOpen_Implementation()
{
	if (OpenTimeline)
	{
		OpenTimeline->PlayFromStart();
	}
}

void ADFDoor::OnOpenTimelineUpdate(const float Alpha)
{
	if (!Mesh)
	{
		return;
	}
	const float Yaw = FMath::Lerp(ClosedMeshYaw, ClosedMeshYaw + OpenMeshYaw, Alpha);
	FRotator R = Mesh->GetRelativeRotation();
	R.Yaw = Yaw;
	Mesh->SetRelativeRotation(R);
}

void ADFDoor::OnOpenTimelineFinished()
{
}

void ADFDoor::OnWorldGameplayEvent(const FGameplayTag EventTag, AActor* /*EventSource*/)
{
	if (!RemoteOpenEventTag.IsValid() || !EventTag.IsValid())
	{
		return;
	}
	if (!EventTag.MatchesTag(RemoteOpenEventTag))
	{
		return;
	}
	if (!HasAuthority())
	{
		return;
	}
	UnlockDoor();
	OpenDoor();
}
