// Source/DungeonForged/Private/Combat/UDFCombatStateLibrary.cpp
#include "Combat/UDFCombatStateLibrary.h"
#include "ADFDungeonManager.h"
#include "Characters/ADFEnemyBase.h"
#include "Data/DFDataTableStructs.h"
#include "Engine/GameInstance.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Data/UDFCombatTuningData.h"
#include "DFAssetManager.h"
#include "Engine/World.h"
#include "GAS/DFGameplayTags.h"
#include "TimerManager.h"

namespace
{
TMap<TWeakObjectPtr<AActor>, FTimerHandle> GCombatExitTimers;
constexpr float DefaultCombatExitDelay = 4.f;

float ResolveCombatExitDelay(const UObject* WorldContextObject)
{
	if (const UDFCombatTuningData* const Tuning = UDFAssetManager::Get().GetCombatTuningData())
	{
		return FMath::Max(0.5f, Tuning->CombatExitDelay);
	}
	return DefaultCombatExitDelay;
}

void RemoveCombatTag(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}
	if (UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor))
	{
		if (FDFGameplayTags::State_InCombat.IsValid())
		{
			ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_InCombat, 1);
		}
	}
	GCombatExitTimers.Remove(Actor);
}
} // namespace

static FOnDFRoomCleared GOnDFRoomCleared;

FOnDFRoomCleared& UDFCombatStateLibrary::GetOnRoomClearedDelegate()
{
	return GOnDFRoomCleared;
}

void UDFCombatStateLibrary::NotifyRoomCleared(UObject* const WorldContextObject, const bool bFloorCleared)
{
	(void)WorldContextObject;
	(void)bFloorCleared;
	GOnDFRoomCleared.Broadcast();
}

bool UDFCombatStateLibrary::IsLastEnemyInRoom(UObject* const WorldContextObject, AActor* const DyingEnemy)
{
	(void)WorldContextObject;
	if (!IsValid(DyingEnemy))
	{
		return false;
	}
	if (const ADFEnemyBase* const Enemy = Cast<ADFEnemyBase>(DyingEnemy))
	{
		if (Enemy->GetEnemyTier() == EEnemyTier::Boss)
		{
			return false;
		}
	}
	if (UWorld* const World = DyingEnemy->GetWorld())
	{
		if (UGameInstance* const GI = World->GetGameInstance())
		{
			if (UDFDungeonManager* const DM = GI->GetSubsystem<UDFDungeonManager>())
			{
				return DM->EnemiesRemaining <= 1 && DM->SpawnedEnemies.Contains(DyingEnemy);
			}
		}
	}
	return false;
}

float UDFCombatStateLibrary::GetCombatExitDelay(const UObject* WorldContextObject)
{
	(void)WorldContextObject;
	return ResolveCombatExitDelay(WorldContextObject);
}

void UDFCombatStateLibrary::NotifyCombatActivity(UObject* WorldContextObject, AActor* CombatActor)
{
	if (!IsValid(CombatActor) || !FDFGameplayTags::State_InCombat.IsValid())
	{
		return;
	}
	UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(CombatActor);
	if (!ASC)
	{
		return;
	}
	ASC->AddLooseGameplayTag(FDFGameplayTags::State_InCombat, 1);

	UWorld* const World = CombatActor->GetWorld();
	if (!World)
	{
		return;
	}
	FTimerHandle& Handle = GCombatExitTimers.FindOrAdd(CombatActor);
	World->GetTimerManager().ClearTimer(Handle);
	const float Delay = ResolveCombatExitDelay(WorldContextObject);
	World->GetTimerManager().SetTimer(
		Handle,
		FTimerDelegate::CreateStatic(&RemoveCombatTag, CombatActor),
		Delay,
		false);
}

bool UDFCombatStateLibrary::IsActorInCombat(UObject* WorldContextObject, AActor* CombatActor)
{
	(void)WorldContextObject;
	if (!IsValid(CombatActor) || !FDFGameplayTags::State_InCombat.IsValid())
	{
		return false;
	}
	if (UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(CombatActor))
	{
		return ASC->HasMatchingGameplayTag(FDFGameplayTags::State_InCombat);
	}
	return false;
}

void UDFCombatStateLibrary::ForceExitCombat(UObject* WorldContextObject, AActor* CombatActor)
{
	(void)WorldContextObject;
	if (!IsValid(CombatActor))
	{
		return;
	}
	if (UWorld* const World = CombatActor->GetWorld())
	{
		if (FTimerHandle* const H = GCombatExitTimers.Find(CombatActor))
		{
			World->GetTimerManager().ClearTimer(*H);
		}
	}
	RemoveCombatTag(CombatActor);
}
