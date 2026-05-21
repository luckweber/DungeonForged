// Source/DungeonForged/Private/Combat/UDFStyleRatingComponent.cpp
#include "Combat/UDFStyleRatingComponent.h"
#include "Engine/World.h"

UDFStyleRatingComponent::UDFStyleRatingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UDFStyleRatingComponent::TickComponent(const float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (DecayPerSecond <= 0.f || Score <= 0.f)
	{
		return;
	}
	const EDFStyleRank OldRank = CurrentRank;
	Score = FMath::Max(0.f, Score - DecayPerSecond * DeltaTime);
	RecalculateRank();
	if (CurrentRank != OldRank)
	{
		OnStyleRankChanged.Broadcast(CurrentRank, Score);
	}
}

void UDFStyleRatingComponent::RecordRaw(const float Delta)
{
	if (Delta <= 0.f)
	{
		return;
	}
	const EDFStyleRank OldRank = CurrentRank;
	Score += Delta;
	RecalculateRank();
	if (CurrentRank != OldRank)
	{
		OnStyleRankChanged.Broadcast(CurrentRank, Score);
	}
}

void UDFStyleRatingComponent::RecordMove(const FGameplayTag MoveTag, const float BaseValue)
{
	if (!MoveTag.IsValid() || BaseValue <= 0.f)
	{
		return;
	}
	UWorld* const W = GetWorld();
	const float Now = W ? W->GetTimeSeconds() : 0.f;
	PruneOldEvents(Now);

	float Multiplier = 1.f;
	for (const FDFStyleEvent& Ev : RecentEvents)
	{
		if (Ev.MoveTag == MoveTag)
		{
			Multiplier = RepeatPenaltyMultiplier;
			break;
		}
	}

	const float Delta = BaseValue * Multiplier;
	FDFStyleEvent Ev;
	Ev.MoveTag = MoveTag;
	Ev.TimeSeconds = Now;
	Ev.ScoreDelta = Delta;
	RecentEvents.Add(Ev);
	RecordRaw(Delta);
}

void UDFStyleRatingComponent::NotifyDamageReceived(const float Amount)
{
	(void)Amount;
	if (!bDropOnDamage)
	{
		return;
	}
	const int32 RankIdx = static_cast<int32>(CurrentRank);
	if (RankIdx <= 0)
	{
		return;
	}
	const EDFStyleRank OldRank = CurrentRank;
	CurrentRank = static_cast<EDFStyleRank>(RankIdx - 1);
	if (RankThresholds.IsValidIndex(static_cast<int32>(CurrentRank)))
	{
		Score = RankThresholds[static_cast<int32>(CurrentRank)];
	}
	if (CurrentRank != OldRank)
	{
		OnStyleRankChanged.Broadcast(CurrentRank, Score);
	}
}

void UDFStyleRatingComponent::PruneOldEvents(const float Now)
{
	RecentEvents.RemoveAll([Now, this](const FDFStyleEvent& Ev)
	{
		return (Now - Ev.TimeSeconds) > RepeatPenaltyWindow;
	});
}

void UDFStyleRatingComponent::RecalculateRank()
{
	if (RankThresholds.Num() == 0)
	{
		CurrentRank = EDFStyleRank::D;
		return;
	}
	EDFStyleRank NewRank = EDFStyleRank::D;
	for (int32 i = 0; i < RankThresholds.Num(); ++i)
	{
		if (Score >= RankThresholds[i])
		{
			NewRank = static_cast<EDFStyleRank>(FMath::Clamp(i, 0, static_cast<int32>(EDFStyleRank::SSS)));
		}
	}
	CurrentRank = NewRank;
}
