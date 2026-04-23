// Source/DungeonForged/Private/Combat/UDFComboPointsComponent.cpp
#include "Combat/UDFComboPointsComponent.h"

UDFComboPointsComponent::UDFComboPointsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDFComboPointsComponent::AddComboPoints(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	CurrentComboPoints = FMath::Clamp(CurrentComboPoints + Amount, 0, MaxComboPoints);
	BroadcastIfChanged();
}

bool UDFComboPointsComponent::SpendComboPoints(int32 Amount)
{
	if (Amount <= 0 || CurrentComboPoints < Amount)
	{
		return false;
	}
	CurrentComboPoints = FMath::Max(0, CurrentComboPoints - Amount);
	BroadcastIfChanged();
	return true;
}

void UDFComboPointsComponent::ResetComboPoints()
{
	CurrentComboPoints = 0;
	BroadcastIfChanged();
}

void UDFComboPointsComponent::BroadcastIfChanged()
{
	OnComboPointsChanged.Broadcast(CurrentComboPoints, MaxComboPoints);
}
