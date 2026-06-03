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

namespace DFAbilityDraft
{
static bool CollectGrantedAbilityTags(const UDFRunManager* RM, FGameplayTagContainer& OutTags)
{
	OutTags.Reset();
	if (!RM || !RM->AbilityDataTable)
	{
		return false;
	}
	for (const FName& RowName : RM->GetCurrentRunState().GrantedAbilities)
	{
		if (const FDFAbilityTableRow* const Row =
				RM->AbilityDataTable->FindRow<FDFAbilityTableRow>(RowName, TEXT("CollectGrantedAbilityTags"), false))
		{
			if (Row->AbilityTag.IsValid())
			{
				OutTags.AddTag(Row->AbilityTag);
			}
		}
	}
	return OutTags.Num() > 0;
}

static bool PassesDraftFilters(const FDFAbilityTableRow& Row, const FGameplayTagContainer& GrantedTags)
{
	if (!Row.ExcludedIfGrantedTags.IsEmpty() && GrantedTags.HasAny(Row.ExcludedIfGrantedTags))
	{
		return false;
	}
	if (!Row.RequiresSynergyTags.IsEmpty() && !GrantedTags.HasAny(Row.RequiresSynergyTags))
	{
		return false;
	}
	return true;
}

static EItemRarity PickWeightedRarity(FRandomStream& Rng)
{
	const float T = Rng.FRand();
	if (T < 0.60f)
	{
		return EItemRarity::Common;
	}
	if (T < 0.85f)
	{
		return EItemRarity::Uncommon;
	}
	if (T < 0.97f)
	{
		return EItemRarity::Rare;
	}
	if (T < 0.99f)
	{
		return EItemRarity::Epic;
	}
	return EItemRarity::Legendary;
}
} // namespace DFAbilityDraft

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

TArray<FDFAbilityRolledChoice> UDFAbilitySelectionSubsystem::RollAbilityChoices(const int32 Count)
{
	TArray<FDFAbilityRolledChoice> Out;
	SyncHistoryFromRun();

	UDataTable* const Tbl = ResolveAbilityTable();
	if (!Tbl || Count <= 0)
	{
		return Out;
	}

	const UDFRunManager* const RM =
		GetWorld() && GetWorld()->GetGameInstance()
			? GetWorld()->GetGameInstance()->GetSubsystem<UDFRunManager>()
			: nullptr;
	FGameplayTagContainer GrantedTags;
	DFAbilityDraft::CollectGrantedAbilityTags(RM, GrantedTags);

	int32 SeedMix = FMath::Rand() * 7919 + FPlatformTime::Cycles();
	if (RM)
	{
		SeedMix ^= RM->GetCurrentRunState().CurrentFloor * 19603;
		SeedMix ^= static_cast<int32>(RM->GetCurrentRunState().RunStartTime);
	}
	FRandomStream Rng(SeedMix);

	TMap<EItemRarity, TArray<FName>> ByRarity;
	Tbl->ForeachRow<FDFAbilityTableRow>(
		TEXT("RollAbilityChoices"),
		[&](const FName& Key, const FDFAbilityTableRow& Row)
		{
			if (PlayerAbilityHistory.Contains(Key) || !Row.AbilityClass)
			{
				return;
			}
			if (!DFAbilityDraft::PassesDraftFilters(Row, GrantedTags))
			{
				return;
			}
			ByRarity.FindOrAdd(Row.Rarity).AddUnique(Key);
		});

	const TArray<EItemRarity> FallbackOrder = {
		EItemRarity::Common,
		EItemRarity::Uncommon,
		EItemRarity::Rare,
		EItemRarity::Epic,
		EItemRarity::Legendary};

	TSet<FName> Picked;
	for (int32 i = 0; i < Count; ++i)
	{
		const EItemRarity FirstTry = DFAbilityDraft::PickWeightedRarity(Rng);
		bool bDone = false;
		for (int32 Round = 0; Round < 2 && !bDone; ++Round)
		{
			const EItemRarity RarityWant = (Round == 0) ? FirstTry : EItemRarity::Common;
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
			if (bDone)
			{
				break;
			}
			for (const EItemRarity R : FallbackOrder)
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
				if (bDone)
				{
					break;
				}
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
	(void)Player;
	RM->AddRunGold(SkipGoldReward);
	SyncHistoryFromRun();
}
