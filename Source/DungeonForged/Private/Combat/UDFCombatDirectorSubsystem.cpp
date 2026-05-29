// Source/DungeonForged/Private/Combat/UDFCombatDirectorSubsystem.cpp
#include "Combat/UDFCombatDirectorSubsystem.h"
#include "AI/ADFAIController.h"
#include "AI/UDFEnemyArchetypeLibrary.h"
#include "Characters/ADFEnemyBase.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

int32 UDFCombatDirectorSubsystem::GetArchetypeAttackPriority(const EDFEnemyArchetype Archetype)
{
	return UDFEnemyArchetypeLibrary::GetMeleeAttackTokenPriority(Archetype);
}

int32 UDFCombatDirectorSubsystem::GetMeleePriority(ADFEnemyBase* const Enemy) const
{
	return Enemy ? GetArchetypeAttackPriority(Enemy->GetEnemyArchetype()) : 0;
}

int32 UDFCombatDirectorSubsystem::GetRangedPriority(ADFEnemyBase* const Enemy) const
{
	return Enemy ? UDFEnemyArchetypeLibrary::GetRangedCastTokenPriority(Enemy->GetEnemyArchetype()) : 0;
}

int32 UDFCombatDirectorSubsystem::GetTelegraphPriority(ADFEnemyBase* const Enemy) const
{
	return Enemy
		? UDFEnemyArchetypeLibrary::GetTelegraphPriority(Enemy->GetEnemyArchetype(), Enemy->GetEnemyTier())
		: 0;
}

void UDFCombatDirectorSubsystem::PruneInvalid()
{
	RegisteredEnemies.RemoveAll([](const TWeakObjectPtr<ADFEnemyBase>& W) { return !W.IsValid(); });
	ActiveAttackers.RemoveAll([](const TWeakObjectPtr<ADFEnemyBase>& W) { return !W.IsValid(); });
	ActiveRangedCasters.RemoveAll([](const TWeakObjectPtr<ADFEnemyBase>& W) { return !W.IsValid(); });
	ActiveTelegraphers.RemoveAll([](const TWeakObjectPtr<ADFEnemyBase>& W) { return !W.IsValid(); });
}

bool UDFCombatDirectorSubsystem::RequestPrioritySlotMutable(
	TArray<TWeakObjectPtr<ADFEnemyBase>>& Active,
	ADFEnemyBase* const Enemy,
	const int32 MaxSlots,
	const int32 Priority,
	TFunctionRef<int32(ADFEnemyBase*)> PriorityFn) const
{
	if (!IsValid(Enemy) || MaxSlots <= 0)
	{
		return false;
	}
	for (const TWeakObjectPtr<ADFEnemyBase>& W : Active)
	{
		if (W.Get() == Enemy)
		{
			return true;
		}
	}
	if (Active.Num() < MaxSlots)
	{
		Active.Add(Enemy);
		return true;
	}
	int32 LowestPriority = MAX_int32;
	int32 LowestIdx = INDEX_NONE;
	for (int32 I = 0; I < Active.Num(); ++I)
	{
		if (ADFEnemyBase* const A = Active[I].Get())
		{
			const int32 P = PriorityFn(A);
			if (P < LowestPriority)
			{
				LowestPriority = P;
				LowestIdx = I;
			}
		}
	}
	if (LowestIdx == INDEX_NONE || Priority <= LowestPriority)
	{
		return false;
	}
	Active.RemoveAt(LowestIdx);
	Active.Add(Enemy);
	return true;
}

void UDFCombatDirectorSubsystem::ReleaseFromSlot(TArray<TWeakObjectPtr<ADFEnemyBase>>& Active, ADFEnemyBase* const Enemy)
{
	Active.RemoveAll([Enemy](const TWeakObjectPtr<ADFEnemyBase>& W) { return W.Get() == Enemy; });
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
	ReleaseRangedCastToken(Enemy);
	ReleaseTelegraphSlot(Enemy);
}

bool UDFCombatDirectorSubsystem::RequestAttackToken(ADFEnemyBase* const Enemy)
{
	if (!IsValid(Enemy))
	{
		return false;
	}
	PruneInvalid();
	return RequestPrioritySlotMutable(
		ActiveAttackers, Enemy, MaxAttackTokens, GetMeleePriority(Enemy),
		[this](ADFEnemyBase* const A) { return GetMeleePriority(A); });
}

void UDFCombatDirectorSubsystem::ReleaseAttackToken(ADFEnemyBase* const Enemy)
{
	ReleaseFromSlot(ActiveAttackers, Enemy);
}

bool UDFCombatDirectorSubsystem::RequestRangedCastToken(ADFEnemyBase* const Enemy)
{
	if (!IsValid(Enemy))
	{
		return false;
	}
	PruneInvalid();
	if (MaxRangedCastTokens <= 0)
	{
		return true;
	}
	return RequestPrioritySlotMutable(
		ActiveRangedCasters, Enemy, MaxRangedCastTokens, GetRangedPriority(Enemy),
		[this](ADFEnemyBase* const A) { return GetRangedPriority(A); });
}

void UDFCombatDirectorSubsystem::ReleaseRangedCastToken(ADFEnemyBase* const Enemy)
{
	ReleaseFromSlot(ActiveRangedCasters, Enemy);
}

bool UDFCombatDirectorSubsystem::CanEnemyTelegraph(ADFEnemyBase* const Enemy) const
{
	if (!IsValid(Enemy))
	{
		return false;
	}
	for (const TWeakObjectPtr<ADFEnemyBase>& W : ActiveTelegraphers)
	{
		if (W.Get() == Enemy)
		{
			return true;
		}
	}
	if (ActiveTelegraphers.Num() < MaxConcurrentTelegraphs)
	{
		return true;
	}
	const int32 RequestP = GetTelegraphPriority(Enemy);
	int32 LowestPriority = MAX_int32;
	for (const TWeakObjectPtr<ADFEnemyBase>& W : ActiveTelegraphers)
	{
		if (ADFEnemyBase* const A = W.Get())
		{
			LowestPriority = FMath::Min(LowestPriority, GetTelegraphPriority(A));
		}
	}
	return RequestP > LowestPriority;
}

bool UDFCombatDirectorSubsystem::RequestTelegraphSlot(ADFEnemyBase* const Enemy)
{
	if (!IsValid(Enemy))
	{
		return false;
	}
	PruneInvalid();
	return RequestPrioritySlotMutable(
		ActiveTelegraphers, Enemy, MaxConcurrentTelegraphs, GetTelegraphPriority(Enemy),
		[this](ADFEnemyBase* const A) { return GetTelegraphPriority(A); });
}

void UDFCombatDirectorSubsystem::ReleaseTelegraphSlot(ADFEnemyBase* const Enemy)
{
	ReleaseFromSlot(ActiveTelegraphers, Enemy);
}

bool UDFCombatDirectorSubsystem::CanSpawnBossMinion(const int32 Count) const
{
	if (MaxBossMinionBudget <= 0)
	{
		return true;
	}
	return ActiveBossMinionCount + FMath::Max(1, Count) <= MaxBossMinionBudget;
}

void UDFCombatDirectorSubsystem::RegisterBossMinion(const int32 Count)
{
	ActiveBossMinionCount += FMath::Max(1, Count);
}

void UDFCombatDirectorSubsystem::UnregisterBossMinion(const int32 Count)
{
	ActiveBossMinionCount = FMath::Max(0, ActiveBossMinionCount - FMath::Max(1, Count));
}

int32 UDFCombatDirectorSubsystem::GetActiveAttackerCount() const
{
	return ActiveAttackers.Num();
}

int32 UDFCombatDirectorSubsystem::GetActiveRangedCasterCount() const
{
	return ActiveRangedCasters.Num();
}

int32 UDFCombatDirectorSubsystem::GetActiveTelegraphCount() const
{
	return ActiveTelegraphers.Num();
}

int32 UDFCombatDirectorSubsystem::GetActiveBossMinionCount() const
{
	return ActiveBossMinionCount;
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
	const float RangeSq = FMath::Square(FMath::Max(100.f, RangeCm));
	int32 MaxNearAnyPlayer = 0;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* const PC = It->Get();
		APawn* const Pawn = PC ? PC->GetPawn() : nullptr;
		if (!IsValid(Pawn))
		{
			continue;
		}
		int32 NearCount = 0;
		for (const TWeakObjectPtr<ADFEnemyBase>& W : RegisteredEnemies)
		{
			if (ADFEnemyBase* const E = W.Get())
			{
				if (FVector::DistSquared(E->GetActorLocation(), Pawn->GetActorLocation()) <= RangeSq)
				{
					++NearCount;
				}
			}
		}
		MaxNearAnyPlayer = FMath::Max(MaxNearAnyPlayer, NearCount);
	}
	return MaxNearAnyPlayer;
}

void UDFCombatDirectorSubsystem::PropagatePackAlert(
	ADFEnemyBase* const SourceEnemy,
	AActor* const TargetPlayer,
	const FVector AlertLocation,
	const float RadiusCm)
{
	if (!IsValid(SourceEnemy) || RadiusCm <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	PruneInvalid();
	const float RadiusSq = FMath::Square(RadiusCm);
	const FVector SourceLocation = SourceEnemy->GetActorLocation();
	for (const TWeakObjectPtr<ADFEnemyBase>& W : RegisteredEnemies)
	{
		ADFEnemyBase* const Ally = W.Get();
		if (!IsValid(Ally) || Ally == SourceEnemy)
		{
			continue;
		}
		if (FVector::DistSquared(Ally->GetActorLocation(), SourceLocation) > RadiusSq)
		{
			continue;
		}
		if (ADFAIController* const AllyAI = Cast<ADFAIController>(Ally->GetController()))
		{
			AllyAI->ReceivePackAlert(TargetPlayer, AlertLocation, true);
		}
	}
}
