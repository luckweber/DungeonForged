// Source/DungeonForged/Private/AI/UDFAILibrary.cpp
#include "AI/UDFAILibrary.h"
#include "AI/UDFAIThreatComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "CollisionQueryParams.h"
#include "Engine/Engine.h"
#include "WorldCollision.h"

namespace
{
bool HasLineOfSightToTarget(UWorld* const World, const AActor* const From, AActor* const Target)
{
	if (!World || !IsValid(From) || !IsValid(Target))
	{
		return false;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DF_AI_HostileLOS), true, From);
	Params.AddIgnoredActor(Target);
	FHitResult Hit;
	return !World->LineTraceSingleByChannel(
		Hit,
		From->GetActorLocation() + FVector(0.f, 0.f, 50.f),
		Target->GetActorLocation() + FVector(0.f, 0.f, 50.f),
		ECC_Visibility,
		Params);
}
} // namespace

bool UDFAILibrary::IsValidHostilePlayerTarget(AActor* const Target)
{
	if (!IsValid(Target))
	{
		return false;
	}
	const APawn* const Pawn = Cast<APawn>(Target);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return false;
	}
	UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!ASC)
	{
		return true;
	}
	if (FDFGameplayTags::State_Dead.IsValid() && ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Dead))
	{
		return false;
	}
	const FGameplayAttribute HealthAttribute = UDFAttributeSet::GetHealthAttribute();
	return !HealthAttribute.IsValid() || ASC->GetNumericAttribute(HealthAttribute) > 0.f;
}

AActor* UDFAILibrary::FindNearestHostilePlayerTarget(
	const UObject* const WorldContextObject,
	const FVector Origin,
	const float MaxRadius,
	AActor* PreferredTarget,
	const bool bRequireLineOfSight,
	AActor* LineOfSightFrom)
{
	UWorld* const World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	if (!World || MaxRadius <= 0.f)
	{
		return nullptr;
	}
	const float MaxRadiusSq = FMath::Square(MaxRadius);
	AActor* Best = nullptr;
	float BestDistSq = MaxRadiusSq;
	const AActor* const LOSFrom = LineOfSightFrom ? LineOfSightFrom : PreferredTarget;

	auto Consider = [&](AActor* const Candidate)
	{
		if (!IsValidHostilePlayerTarget(Candidate))
		{
			return;
		}
		const float DistSq = FVector::DistSquared(Origin, Candidate->GetActorLocation());
		if (DistSq > MaxRadiusSq)
		{
			return;
		}
		if (bRequireLineOfSight && !HasLineOfSightToTarget(World, LOSFrom, Candidate))
		{
			return;
		}
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Candidate;
		}
	};

	if (IsValidHostilePlayerTarget(PreferredTarget))
	{
		const float PreferredDistSq = FVector::DistSquared(Origin, PreferredTarget->GetActorLocation());
		if (PreferredDistSq <= MaxRadiusSq
			&& (!bRequireLineOfSight || HasLineOfSightToTarget(World, LOSFrom, PreferredTarget)))
		{
			Best = PreferredTarget;
			BestDistSq = PreferredDistSq;
		}
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* const PC = It->Get();
		if (!PC)
		{
			continue;
		}
		Consider(PC->GetPawn());
	}

	return Best;
}

ACharacter* UDFAILibrary::FindNearestHostilePlayerCharacter(
	const UObject* const WorldContextObject,
	const FVector Origin,
	const float MaxRadius,
	AActor* const IgnoreActor,
	AActor* const PreferredTarget)
{
	if (AActor* const Target = FindNearestHostilePlayerTarget(
		WorldContextObject, Origin, MaxRadius, PreferredTarget, false, nullptr))
	{
		if (Target != IgnoreActor)
		{
			return Cast<ACharacter>(Target);
		}
	}

	UWorld* const World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	if (!World)
	{
		return nullptr;
	}

	ACharacter* Best = nullptr;
	const float MaxRadiusSq = FMath::Square(MaxRadius);
	float BestDistSq = MaxRadiusSq;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* const PC = It->Get();
		APawn* const Pawn = PC ? PC->GetPawn() : nullptr;
		if (!IsValidHostilePlayerTarget(Pawn) || Pawn == IgnoreActor)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Origin, Pawn->GetActorLocation());
		if (DistSq <= BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Cast<ACharacter>(Pawn);
		}
	}
	return Best;
}

AActor* UDFAILibrary::FindBestHostilePlayerTarget(
	const UObject* const WorldContextObject,
	AActor* const SelfActor,
	const FVector Origin,
	const float MaxRadius,
	AActor* PreferredTarget,
	const bool bRequireLineOfSight)
{
	if (IsValid(SelfActor))
	{
		if (UDFAIThreatComponent* const Threat = SelfActor->FindComponentByClass<UDFAIThreatComponent>())
		{
			if (AActor* const ThreatTarget = Threat->GetBestThreatTarget(
				Origin, MaxRadius, bRequireLineOfSight, SelfActor))
			{
				return ThreatTarget;
			}
		}
	}
	return FindNearestHostilePlayerTarget(
		WorldContextObject, Origin, MaxRadius, PreferredTarget, bRequireLineOfSight, SelfActor);
}
