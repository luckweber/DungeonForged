// Source/DungeonForged/Private/Combat/UDFProjectileSweepComponent.cpp
#include "Combat/UDFProjectileSweepComponent.h"

#include "Components/SphereComponent.h"
#include "DFAssetManager.h"
#include "Data/UDFCombatTuningData.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UDFProjectileSweepComponent::UDFProjectileSweepComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UDFProjectileSweepComponent::BeginPlay()
{
	Super::BeginPlay();
	if (const UDFCombatTuningData* const Tuning = UDFAssetManager::Get().GetCombatTuningData())
	{
		SweepSubSteps = FMath::Clamp(Tuning->ProjectileSweepSubSteps, 1, 8);
	}
	if (AActor* const Owner = GetOwner())
	{
		TrackedSphere = Cast<USphereComponent>(Owner->GetRootComponent());
	}
	ResetTraceSegment();
}

void UDFProjectileSweepComponent::ResetTraceSegment()
{
	if (AActor* const Owner = GetOwner())
	{
		LastTraceLocation = Owner->GetActorLocation();
		bHasLastTraceLocation = true;
	}
}

void UDFProjectileSweepComponent::TickComponent(
	const float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	(void)DeltaTime;
	if (!bSweepEnabled || !TrackedSphere || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	const FVector End = GetOwner()->GetActorLocation();
	if (!bHasLastTraceLocation)
	{
		LastTraceLocation = End;
		bHasLastTraceLocation = true;
		return;
	}
	if (FVector::DistSquared(LastTraceLocation, End) <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	PerformSweep(LastTraceLocation, End);
	LastTraceLocation = End;
}

void UDFProjectileSweepComponent::PerformSweep(const FVector& Start, const FVector& End)
{
	UWorld* const W = GetWorld();
	AActor* const Owner = GetOwner();
	if (!W || !Owner || !TrackedSphere)
	{
		return;
	}
	const float Radius = TrackedSphere->GetScaledSphereRadius();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ProjectileSweep), false, Owner);
	Params.AddIgnoredActor(Owner);
	if (APawn* const Inst = Owner->GetInstigator())
	{
		Params.AddIgnoredActor(Inst);
	}
	const int32 Steps = FMath::Clamp(SweepSubSteps, 1, 8);
	for (int32 Step = 1; Step <= Steps; ++Step)
	{
		const float Alpha = static_cast<float>(Step) / static_cast<float>(Steps);
		const FVector Pos = FMath::Lerp(Start, End, Alpha);
		TArray<FHitResult> Hits;
		if (!W->SweepMultiByChannel(
			Hits,
			Pos,
			Pos,
			FQuat::Identity,
			ECC_Pawn,
			FCollisionShape::MakeSphere(Radius),
			Params))
		{
			continue;
		}
		for (const FHitResult& Hit : Hits)
		{
			AActor* const Other = Hit.GetActor();
			if (!IsValid(Other) || Other == Owner || Other == Owner->GetInstigator())
			{
				continue;
			}
			OnSweepHit.Broadcast(Hit, TrackedSphere.Get());
			return;
		}
	}
}
