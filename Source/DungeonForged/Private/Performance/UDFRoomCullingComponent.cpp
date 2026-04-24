// Source/DungeonForged/Private/Performance/UDFRoomCullingComponent.cpp
#include "Performance/UDFRoomCullingComponent.h"
#include "Performance/UDFPerformanceSubsystem.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"

UDFRoomCullingComponent::UDFRoomCullingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDFRoomCullingComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* const W = GetWorld())
	{
		if (UDFPerformanceSubsystem* const Ps = W->GetSubsystem<UDFPerformanceSubsystem>())
		{
			Ps->RegisterRoomCulling(this);
		}
	}
}

void UDFRoomCullingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* const W = GetWorld())
	{
		if (UDFPerformanceSubsystem* const Ps = W->GetSubsystem<UDFPerformanceSubsystem>())
		{
			Ps->UnregisterRoomCulling(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

FVector UDFRoomCullingComponent::GetCullOrigin() const
{
	if (AActor* const O = GetOwner())
	{
		return O->GetActorLocation();
	}
	return FVector::ZeroVector;
}

void UDFRoomCullingComponent::UpdateVisibilityForPlayer(const FVector& PlayerLocation)
{
	const float Thresh = FMath::Max(1.f, CullDistance) * 1.5f;
	const float D = FVector::Dist(GetCullOrigin(), PlayerLocation);
	const bool bWantVisible = D <= Thresh;
	if (bWantVisible == bIsVisible)
	{
		return;
	}
	SetRoomVisible(bWantVisible);
}

void UDFRoomCullingComponent::SetRoomVisible(const bool bVisible)
{
	bIsVisible = bVisible;
	for (TObjectPtr<AActor>& A : RoomActors)
	{
		if (!IsValid(A))
		{
			continue;
		}
		A->SetActorHiddenInGame(!bVisible);
		A->SetActorEnableCollision(bVisible);
		A->SetActorTickEnabled(bVisible);

		if (UBehaviorTreeComponent* const BT = A->FindComponentByClass<UBehaviorTreeComponent>())
		{
			if (bVisible)
			{
				BT->ResumeLogic(TEXT("RoomCull"));
			}
			else
			{
				BT->PauseLogic(TEXT("RoomCull"));
			}
		}

		TArray<UNiagaraComponent*> NiagaraComps;
		A->GetComponents<UNiagaraComponent>(NiagaraComps);
		for (UNiagaraComponent* const N : NiagaraComps)
		{
			if (!N)
			{
				continue;
			}
			if (bVisible)
			{
				// Leave off; gameplay re-activates if needed
			}
			else
			{
				N->DeactivateImmediate();
			}
		}
	}
}
