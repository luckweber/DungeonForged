// Source/DungeonForged/Private/Combat/UDFCombatDirectorSubsystem.cpp
#include "Combat/UDFCombatDirectorSubsystem.h"
#include "Characters/ADFEnemyBase.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

int32 UDFCombatDirectorSubsystem::GetArchetypeAttackPriority(const EDFEnemyArchetype Archetype)
{
	switch (Archetype)
	{
	case EDFEnemyArchetype::Tank: return 100;
	case EDFEnemyArchetype::Sniper: return 90;
	case EDFEnemyArchetype::Caster: return 80;
	case EDFEnemyArchetype::Berserker: return 70;
	default: return 50;
	}
}

void UDFCombatDirectorSubsystem::PruneInvalid()
{
	RegisteredEnemies.RemoveAll([](const TWeakObjectPtr<ADFEnemyBase>& W) { return !W.IsValid(); });
	ActiveAttackers.RemoveAll([](const TWeakObjectPtr<ADFEnemyBase>& W) { return !W.IsValid(); });
}

void UDFCombatDirectorSubsystem::RegisterEnemy(ADFEnemyBase* const Enemy)
{
	if (!IsValid(Enemy))
	{
		return;
	}
	PruneInvalid();
	RegisteredEnemies.AddUnique(Enemy);
}

void UDFCombatDirectorSubsystem::UnregisterEnemy(ADFEnemyBase* const Enemy)
{
	PruneInvalid();
	RegisteredEnemies.RemoveAll([Enemy](const TWeakObjectPtr<ADFEnemyBase>& W) { return W.Get() == Enemy; });
	ReleaseAttackToken(Enemy);
}

bool UDFCombatDirectorSubsystem::RequestAttackToken(ADFEnemyBase* const Enemy)
{
	if (!IsValid(Enemy))
	{
		return false;
	}
	PruneInvalid();
	for (const TWeakObjectPtr<ADFEnemyBase>& W : ActiveAttackers)
	{
		if (W.Get() == Enemy)
		{
			return true;
		}
	}
	if (ActiveAttackers.Num() >= MaxAttackTokens)
	{
		int32 LowestPriority = MAX_int32;
		int32 LowestIdx = INDEX_NONE;
		for (int32 I = 0; I < ActiveAttackers.Num(); ++I)
		{
			if (ADFEnemyBase* const A = ActiveAttackers[I].Get())
			{
				const int32 P = GetEnemyPriority(A);
				if (P < LowestPriority)
				{
					LowestPriority = P;
					LowestIdx = I;
				}
			}
		}
		const int32 RequestP = GetEnemyPriority(Enemy);
		if (LowestIdx == INDEX_NONE || RequestP <= LowestPriority)
		{
			return false;
		}
		ActiveAttackers.RemoveAt(LowestIdx);
	}
	ActiveAttackers.Add(Enemy);
	return true;
}

void UDFCombatDirectorSubsystem::ReleaseAttackToken(ADFEnemyBase* const Enemy)
{
	ActiveAttackers.RemoveAll([Enemy](const TWeakObjectPtr<ADFEnemyBase>& W) { return W.Get() == Enemy; });
}

int32 UDFCombatDirectorSubsystem::GetActiveAttackerCount() const
{
	return ActiveAttackers.Num();
}

int32 UDFCombatDirectorSubsystem::GetRegisteredEnemyCount() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<ADFEnemyBase>& W : RegisteredEnemies)
	{
		if (W.IsValid())
		{
			++N;
		}
	}
	return N;
}

int32 UDFCombatDirectorSubsystem::GetEnemiesNearLocalPlayer(const float RangeCm) const
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		return 0;
	}
	APlayerController* const PC = UGameplayStatics::GetPlayerController(World, 0);
	APawn* const Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return 0;
	}
	const float RangeSq = FMath::Square(FMath::Max(100.f, RangeCm));
	int32 N = 0;
	for (const TWeakObjectPtr<ADFEnemyBase>& W : RegisteredEnemies)
	{
		if (ADFEnemyBase* const E = W.Get())
		{
			if (FVector::DistSquared(E->GetActorLocation(), Pawn->GetActorLocation()) <= RangeSq)
			{
				++N;
			}
		}
	}
	return N;
}

int32 UDFCombatDirectorSubsystem::GetEnemyPriority(ADFEnemyBase* const Enemy) const
{
	return Enemy ? GetArchetypeAttackPriority(Enemy->GetEnemyArchetype()) : 0;
}
