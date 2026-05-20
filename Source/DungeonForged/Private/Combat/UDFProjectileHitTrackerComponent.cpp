// Source/DungeonForged/Private/Combat/UDFProjectileHitTrackerComponent.cpp
#include "Combat/UDFProjectileHitTrackerComponent.h"

UDFProjectileHitTrackerComponent::UDFProjectileHitTrackerComponent()
{
	SetIsReplicatedByDefault(false);
}

bool UDFProjectileHitTrackerComponent::TryRegisterHit(AActor* const Target)
{
	if (!Target)
	{
		return false;
	}
	for (const TWeakObjectPtr<AActor>& Existing : HitTargets)
	{
		if (Existing.Get() == Target)
		{
			return false;
		}
	}
	HitTargets.Add(Target);
	return true;
}

void UDFProjectileHitTrackerComponent::ClearHitHistory()
{
	HitTargets.Reset();
}
