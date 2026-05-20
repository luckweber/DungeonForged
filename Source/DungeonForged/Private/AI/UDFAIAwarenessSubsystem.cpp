// Source/DungeonForged/Private/AI/UDFAIAwarenessSubsystem.cpp
#include "AI/UDFAIAwarenessSubsystem.h"

#include "DungeonForgedModule.h"
#include "Engine/World.h"

void UDFAIAwarenessSubsystem::PruneInvalid()
{
	CurrentlyTelegraphing.RemoveAll([](const TWeakObjectPtr<AActor>& Ptr) { return !Ptr.IsValid(); });
}

void UDFAIAwarenessSubsystem::OnTelegraphBegin(AActor* const Enemy)
{
	if (!IsValid(Enemy))
	{
		return;
	}
	PruneInvalid();
	CurrentlyTelegraphing.AddUnique(Enemy);
	UE_LOG(LogDFAI, Verbose, TEXT("[AI] TelegraphBegin %s count=%d"), *GetNameSafe(Enemy), CurrentlyTelegraphing.Num());
}

void UDFAIAwarenessSubsystem::OnTelegraphEnd(AActor* const Enemy)
{
	if (!IsValid(Enemy))
	{
		return;
	}
	CurrentlyTelegraphing.Remove(Enemy);
	PruneInvalid();
	UE_LOG(LogDFAI, Verbose, TEXT("[AI] TelegraphEnd %s count=%d"), *GetNameSafe(Enemy), CurrentlyTelegraphing.Num());
}

int32 UDFAIAwarenessSubsystem::GetTelegraphingCountWithin(const FVector& Center, const float RadiusCm) const
{
	const float RadiusSq = FMath::Square(FMath::Max(1.f, RadiusCm));
	int32 Count = 0;
	for (const TWeakObjectPtr<AActor>& Ptr : CurrentlyTelegraphing)
	{
		if (AActor* const A = Ptr.Get())
		{
			if (FVector::DistSquared(A->GetActorLocation(), Center) <= RadiusSq)
			{
				++Count;
			}
		}
	}
	return Count;
}
