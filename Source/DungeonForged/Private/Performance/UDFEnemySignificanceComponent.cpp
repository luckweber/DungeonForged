// Source/DungeonForged/Private/Performance/UDFEnemySignificanceComponent.cpp
#include "Performance/UDFEnemySignificanceComponent.h"
#include "AI/UDFAILibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

UDFEnemySignificanceComponent::UDFEnemySignificanceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.35f;
}

void UDFEnemySignificanceComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* const Owner = GetOwner())
	{
		CachedMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
	}
}

void UDFEnemySignificanceComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* const ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UWorld* const World = GetWorld();
	AActor* const Owner = GetOwner();
	if (!World || !Owner)
	{
		return;
	}
	float NearestSq = TNumericLimits<float>::Max();
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (APawn* const Pawn = It->Get() ? It->Get()->GetPawn() : nullptr)
		{
			NearestSq = FMath::Min(NearestSq, FVector::DistSquared(Owner->GetActorLocation(), Pawn->GetActorLocation()));
		}
	}
	const float Dist = FMath::Sqrt(NearestSq);
	int32 Tier = 0;
	if (Dist > LowDetailDistance)
	{
		Tier = 3;
	}
	else if (Dist > MediumDetailDistance)
	{
		Tier = 2;
	}
	else if (Dist > HighDetailDistance)
	{
		Tier = 1;
	}
	if (Tier != CurrentTier)
	{
		CurrentTier = Tier;
		ApplySignificanceTier(Tier);
	}
}

void UDFEnemySignificanceComponent::ApplySignificanceTier(const int32 Tier) const
{
	if (!CachedMesh)
	{
		return;
	}
	switch (Tier)
	{
	case 0:
		CachedMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		CachedMesh->bEnableUpdateRateOptimizations = false;
		break;
	case 1:
		CachedMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
		CachedMesh->bEnableUpdateRateOptimizations = true;
		break;
	case 2:
		CachedMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		CachedMesh->bEnableUpdateRateOptimizations = true;
		break;
	default:
		CachedMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;
		CachedMesh->bEnableUpdateRateOptimizations = true;
		break;
	}
}
