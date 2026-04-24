// Source/DungeonForged/Private/Network/UDFNetworkValidator.cpp

#include "Network/UDFNetworkValidator.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "NavigationSystem.h"

bool UDFNetworkValidator::HasWorldAuthority(UObject const* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return false;
	}
	const UWorld* const W = WorldContextObject->GetWorld();
	if (!W)
	{
		return false;
	}
	return W->GetNetMode() != NM_Client;
}

bool UDFNetworkValidator::ValidateAbilityAuthority(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return false;
	}
	return ASC->GetOwner() && ASC->GetOwner()->HasAuthority();
}

bool UDFNetworkValidator::ValidateEconomyAuthority(UObject const* SourceObject)
{
	if (!SourceObject)
	{
		return false;
	}
	if (const AActor* const A = Cast<AActor>(SourceObject))
	{
		return A->HasAuthority();
	}
	if (const UActorComponent* const C = Cast<UActorComponent>(SourceObject))
	{
		return C->GetOwner() && C->GetOwner()->HasAuthority();
	}
	return SourceObject->GetWorld() && SourceObject->GetWorld()->GetNetMode() != NM_Client;
}

bool UDFNetworkValidator::IsPawnNearNavMesh(UWorld* World, AActor* Pawn, float MaxDistance)
{
	if (!World || !Pawn || MaxDistance <= 0.f)
	{
		return true;
	}
	if (UNavigationSystemV1* const Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
	{
		FNavLocation Loc;
		const FVector Pt = Pawn->GetActorLocation();
		if (Nav->ProjectPointToNavigation(Pt, Loc, FVector(MaxDistance, MaxDistance, MaxDistance)))
		{
			return FVector::Dist(Pt, Loc.Location) <= MaxDistance;
		}
	}
	return false;
}
