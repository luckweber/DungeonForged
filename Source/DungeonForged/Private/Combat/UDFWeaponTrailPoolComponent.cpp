// Source/DungeonForged/Private/Combat/UDFWeaponTrailPoolComponent.cpp
#include "Combat/UDFWeaponTrailPoolComponent.h"

#include "GameFramework/Character.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

UDFWeaponTrailPoolComponent::UDFWeaponTrailPoolComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDFWeaponTrailPoolComponent::BeginPlay()
{
	Super::BeginPlay();
	BuildPool();
}

void UDFWeaponTrailPoolComponent::BuildPool()
{
	Pool.Reset();
	if (!TrailSystem)
	{
		return;
	}
	AActor* const Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	USceneComponent* AttachParent = Owner->GetRootComponent();
	if (ACharacter* const Char = Cast<ACharacter>(Owner))
	{
		if (USkeletalMeshComponent* const Mesh = Char->GetMesh())
		{
			AttachParent = Mesh;
		}
	}
	const int32 Count = FMath::Clamp(PoolSize, 1, 4);
	for (int32 i = 0; i < Count; ++i)
	{
		UNiagaraComponent* const NC = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			AttachParent,
			AttachSocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			false);
		if (NC)
		{
			NC->Deactivate();
			Pool.Add(NC);
		}
	}
}

void UDFWeaponTrailPoolComponent::ActivateTrail()
{
	if (Pool.IsEmpty())
	{
		BuildPool();
	}
	if (Pool.IsEmpty())
	{
		return;
	}
	for (UNiagaraComponent* const NC : Pool)
	{
		if (NC)
		{
			NC->Deactivate();
		}
	}
	ActiveIndex = (ActiveIndex + 1) % Pool.Num();
	if (UNiagaraComponent* const Active = Pool[ActiveIndex])
	{
		Active->Activate(true);
	}
}

void UDFWeaponTrailPoolComponent::DeactivateTrail()
{
	for (UNiagaraComponent* const NC : Pool)
	{
		if (NC)
		{
			NC->Deactivate();
		}
	}
}
