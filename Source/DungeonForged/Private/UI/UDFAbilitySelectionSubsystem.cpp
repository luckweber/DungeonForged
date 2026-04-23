// Source/DungeonForged/Private/UI/UDFAbilitySelectionSubsystem.cpp
#include "UI/UDFAbilitySelectionSubsystem.h"
#include "UI/UDFAbilitySelectionWidget.h"
#include "Run/DFRunManager.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Characters/ADFPlayerState.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerState.h"
#include "Math/UnrealMathUtility.h"

void UDFAbilitySelectionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SyncHistoryFromRun();
}

void UDFAbilitySelectionSubsystem::RegisterActiveSelectionWidget(UDFAbilitySelectionWidget* const Widget)
{
	if (Widget)
	{
		ActiveSelectionWidgets.AddUnique(Widget);
	}
}

void UDFAbilitySelectionSubsystem::UnregisterActiveSelectionWidget(UDFAbilitySelectionWidget* const Widget)
{
	ActiveSelectionWidgets.RemoveAll(
		[Widget](const TWeakObjectPtr<UDFAbilitySelectionWidget>& P) { return P.Get() == Widget; });
}

void UDFAbilitySelectionSubsystem::CloseActiveSelectionWidget()
{
	for (TWeakObjectPtr<UDFAbilitySelectionWidget>& WP : ActiveSelectionWidgets)
	{
		if (UDFAbilitySelectionWidget* const W = WP.Get())
		{
			W->RemoveFromParent();
		}
	}
	ActiveSelectionWidgets.Reset();
}

UDataTable* UDFAbilitySelectionSubsystem::ResolveAbilityTable() const
{
	if (AbilityTable)
	{
		return AbilityTable;
	}
	if (UWorld* const W = GetWorld())
	{
		if (UGameInstance* const GI = W->GetGameInstance())
		{
			if (const UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
			{
				return RM->AbilityDataTable;
			}
		}
	}
	return nullptr;
}

void UDFAbilitySelectionSubsystem::SyncHistoryFromRun()
{
	if (UWorld* const W = GetWorld())
	{
		if (UGameInstance* const GI = W->GetGameInstance())
		{
			if (const UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
			{
				PlayerAbilityHistory = RM->GetCurrentRunState().GrantedAbilities;
				return;
			}
		}
	}
	PlayerAbilityHistory.Reset();
}

static EItemRarity PickWeightedRarity(FRandomStream& Rng)
{
	const float t = Rng.FRand();
	if (t < 0.6f) return EItemRarity::Common;
	if (t < 0.85f) return EItemRarity::Uncommon;
	if (t < 0.97f) return EItemRarity::Rare;
	return EItemRarity::Epic;
}

static EItemRarity RarityToBucket(const EItemRarity R)
{
	if (R == EItemRarity::Legendary)
	{
		return EItemRarity::Epic;
	}
	return R;
}

TArray<FDFAbilityRolledChoice> UDFAbilitySelectionSubsystem::RollAbilityChoices(const int32 Count)
{
	TArray<FDFAbilityRolledChoice> Out;
	SyncHistoryFromRun();

	UDataTable* const Tbl = ResolveAbilityTable();
	if (!Tbl || Count <= 0)
	{
		return Out;
	}
	FRandomStream Rng(FMath::Rand() * 7919 + FPlatformTime::Cycles());

	// Rarity -> row names (available only)
	TMap<EItemRarity, TArray<FName>> ByRarity;
	Tbl->ForeachRow<FDFAbilityTableRow>(TEXT("RollAbilityChoices"), [&](const FName& Key, const FDFAbilityTableRow& Row) {
		if (PlayerAbilityHistory.Contains(Key) || !Row.AbilityClass)
		{
			return;
		}
		ByRarity.FindOrAdd(RarityToBucket(Row.Rarity)).AddUnique(Key);
	});

	const TArray<EItemRarity> FallbackOrder = {
		EItemRarity::Common, EItemRarity::Uncommon, EItemRarity::Rare, EItemRarity::Epic
	};

	TSet<FName> Picked;
	for (int32 i = 0; i < Count; ++i)
	{
		const EItemRarity FirstTry = PickWeightedRarity(Rng);
		bool bDone = false;
		for (int32 Round = 0; Round < 2 && !bDone; ++Round)
		{
			const EItemRarity RarityWant = (Round == 0) ? FirstTry : EItemRarity::Common;
			// 1) try RarityWant
			{
				if (TArray<FName>* Pool = ByRarity.Find(RarityWant))
				{
					Pool->RemoveAll([&Picked](const FName& N) { return Picked.Contains(N); });
					if (Pool->Num() > 0)
					{
						const int32 Ix = Rng.RandRange(0, Pool->Num() - 1);
						const FName N = (*Pool)[Ix];
						if (const FDFAbilityTableRow* R = Tbl->FindRow<FDFAbilityTableRow>(N, TEXT("RollPick"), false))
						{
							Picked.Add(N);
							FDFAbilityRolledChoice C;
							C.RowName = N;
							C.Data = *R;
							Out.Add(C);
							Pool->RemoveAt(Ix);
							bDone = true;
						}
					}
				}
			}
			if (bDone) break;
			// 2) try all buckets
			for (EItemRarity R : FallbackOrder)
			{
				if (TArray<FName>* Pool = ByRarity.Find(R))
				{
					Pool->RemoveAll([&Picked](const FName& N) { return Picked.Contains(N); });
					if (Pool->Num() > 0)
					{
						const int32 Ix = Rng.RandRange(0, Pool->Num() - 1);
						const FName N = (*Pool)[Ix];
						if (const FDFAbilityTableRow* Rr = Tbl->FindRow<FDFAbilityTableRow>(N, TEXT("RollPick2"), false))
						{
							Picked.Add(N);
							FDFAbilityRolledChoice C;
							C.RowName = N;
							C.Data = *Rr;
							Out.Add(C);
							Pool->RemoveAt(Ix);
							bDone = true;
						}
					}
				}
				if (bDone) break;
			}
		}
	}
	return Out;
}

void UDFAbilitySelectionSubsystem::GrantSelectedAbility(const FName AbilityRowName, ADFPlayerCharacter* const Player)
{
	if (AbilityRowName.IsNone() || !IsValid(Player) || !Player->GetPlayerState() || !Player->HasAuthority())
	{
		return;
	}
	UGameInstance* const GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UDFRunManager* const RM = GI ? GI->GetSubsystem<UDFRunManager>() : nullptr;
	ADFPlayerState* const PS = Player->GetPlayerState<ADFPlayerState>();
	if (!RM || !PS)
	{
		return;
	}
	RM->AddAbilityReward(AbilityRowName);
	RM->GrantAbilitiesForCurrentRun(PS);
	SyncHistoryFromRun();
}

void UDFAbilitySelectionSubsystem::SkipSelection(ADFPlayerCharacter* const Player)
{
	UGameInstance* const GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UDFRunManager* const RM = GI ? GI->GetSubsystem<UDFRunManager>() : nullptr;
	if (!RM || !GetWorld() || GetWorld()->GetNetMode() == NM_Client)
	{
		return;
	}
	// Player is unused for run gold; grant path still needs a valid pawn+ASC.
	(void)Player;
	RM->AddRunGold(SkipGoldReward);
	SyncHistoryFromRun();
}
