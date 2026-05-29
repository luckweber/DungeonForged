// Source/DungeonForged/Private/AI/UDFAINoiseLibrary.cpp
#include "AI/UDFAINoiseLibrary.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"

namespace
{
float BandToLoudness(const EDFHitFeedbackBand Band, const bool bWasLethal)
{
	if (bWasLethal)
	{
		return 1.35f;
	}
	switch (Band)
	{
	case EDFHitFeedbackBand::Critical:
	case EDFHitFeedbackBand::Knockback:
		return 1.15f;
	case EDFHitFeedbackBand::Heavy:
		return 1.f;
	default:
		return 0.65f;
	}
}
} // namespace

void UDFAINoiseLibrary::ReportNoiseAtLocation(
	const UObject* const WorldContextObject,
	AActor* const Instigator,
	const FVector Location,
	const float Loudness,
	const float MaxRange,
	const FName Tag)
{
	UWorld* const World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	if (!World || Loudness <= KINDA_SMALL_NUMBER || MaxRange <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	UAISense_Hearing::ReportNoiseEvent(
		World,
		Location,
		Loudness,
		Instigator,
		MaxRange,
		Tag != NAME_None ? Tag : FName(TEXT("DF.Combat")));
}

void UDFAINoiseLibrary::ReportCombatHitNoise(
	AActor* const Instigator,
	const FVector Location,
	const EDFHitFeedbackBand Band,
	const bool bWasLethal)
{
	if (!IsValid(Instigator))
	{
		return;
	}
	const APawn* const InstigatorPawn = Cast<APawn>(Instigator);
	if (!InstigatorPawn || !InstigatorPawn->IsPlayerControlled())
	{
		return;
	}
	const float Loudness = BandToLoudness(Band, bWasLethal);
	const float Range = bWasLethal ? 1800.f : 1300.f;
	ReportNoiseAtLocation(Instigator, Instigator, Location, Loudness, Range, FName(TEXT("DF.Combat.Hit")));
}

void UDFAINoiseLibrary::ReportAbilityNoise(
	AActor* const Instigator,
	const float Loudness,
	const float MaxRange)
{
	if (!IsValid(Instigator))
	{
		return;
	}
	ReportNoiseAtLocation(Instigator, Instigator, Instigator->GetActorLocation(), Loudness, MaxRange, FName(TEXT("DF.Combat.Ability")));
}
