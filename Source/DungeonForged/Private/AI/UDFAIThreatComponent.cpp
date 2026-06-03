// Source/DungeonForged/Private/AI/UDFAIThreatComponent.cpp
#include "AI/UDFAIThreatComponent.h"
#include "AI/UDFAILibrary.h"
#include "Engine/World.h"

UDFAIThreatComponent::UDFAIThreatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f;
	SetIsReplicatedByDefault(false);
}

void UDFAIThreatComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickEnabled(GetOwner() && GetOwner()->HasAuthority());
}

void UDFAIThreatComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* const ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		DecayThreat(DeltaTime);
	}
}

void UDFAIThreatComponent::DecayThreat(const float DeltaTime)
{
	if (ThreatDecayPerSecond <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	TArray<TWeakObjectPtr<AActor>> Stale;
	for (TPair<TWeakObjectPtr<AActor>, float>& Pair : ThreatByTarget)
	{
		if (!Pair.Key.IsValid())
		{
			Stale.Add(Pair.Key);
			continue;
		}
		Pair.Value = FMath::Max(0.f, Pair.Value - ThreatDecayPerSecond * DeltaTime);
		if (Pair.Value <= KINDA_SMALL_NUMBER)
		{
			Stale.Add(Pair.Key);
		}
	}
	for (const TWeakObjectPtr<AActor>& K : Stale)
	{
		ThreatByTarget.Remove(K);
	}
}

void UDFAIThreatComponent::AddThreat(AActor* const Source, const float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValid(Source) || Amount <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	if (!UDFAILibrary::IsValidHostilePlayerTarget(Source))
	{
		return;
	}
	float& Threat = ThreatByTarget.FindOrAdd(Source);
	Threat += Amount * DamageThreatMultiplier;
}

AActor* UDFAIThreatComponent::GetBestThreatTarget(
	const FVector Origin,
	const float MaxRadius,
	const bool bRequireLineOfSight,
	AActor* const LineOfSightFrom) const
{
	UWorld* const World = GetWorld();
	if (!World || MaxRadius <= 0.f)
	{
		return nullptr;
	}
	const float MaxRadiusSq = FMath::Square(MaxRadius);
	AActor* Best = nullptr;
	float BestScore = -1.f;
	const AActor* const LOSFrom = LineOfSightFrom ? LineOfSightFrom : GetOwner();

	for (const TPair<TWeakObjectPtr<AActor>, float>& Pair : ThreatByTarget)
	{
		AActor* const Target = Pair.Key.Get();
		if (!UDFAILibrary::IsValidHostilePlayerTarget(Target))
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Origin, Target->GetActorLocation());
		if (DistSq > MaxRadiusSq)
		{
			continue;
		}
		if (bRequireLineOfSight)
		{
			FCollisionQueryParams Params(SCENE_QUERY_STAT(DF_ThreatLOS), true, GetOwner());
			Params.AddIgnoredActor(Target);
			FHitResult Hit;
			if (World->LineTraceSingleByChannel(
				Hit,
				Origin + FVector(0.f, 0.f, 50.f),
				Target->GetActorLocation() + FVector(0.f, 0.f, 50.f),
				ECC_Visibility,
				Params))
			{
				continue;
			}
		}
		const float Dist = FMath::Sqrt(DistSq);
		const float ProximityBonus = ProximityThreatBias * (1.f - FMath::Clamp(Dist / MaxRadius, 0.f, 1.f));
		const float Score = Pair.Value + ProximityBonus;
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Target;
		}
	}
	return Best;
}
