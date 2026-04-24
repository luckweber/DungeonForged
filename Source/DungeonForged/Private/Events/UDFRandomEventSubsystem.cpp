// Source/DungeonForged/Private/Events/UDFRandomEventSubsystem.cpp

#include "Events/UDFRandomEventSubsystem.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Characters/ADFPlayerState.h"
#include "Data/DFDataTableStructs.h"
#include "DFInventoryComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GAS/Effects/UDFGameplayEffectLibrary.h"
#include "Run/DFRunManager.h"
#include "UI/UDFAbilitySelectionSubsystem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"

void UDFRandomEventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UDFRandomEventSubsystem::ResetUsedEvents()
{
	UsedEvents.Reset();
}

bool UDFRandomEventSubsystem::ShouldTriggerEvent() const
{
	return FMath::FRand() < FMath::Clamp(EventChancePerFloor, 0.f, 1.f);
}

const FDFRandomEventRow* UDFRandomEventSubsystem::RollEvent(int32 const CurrentFloor, FName& OutRowName)
{
	OutRowName = NAME_None;
	if (!EventTable)
	{
		return nullptr;
	}

	struct FEntry
	{
		FName RowName;
		const FDFRandomEventRow* RowPtr;
		float Weight;
	};
	TArray<FEntry> Eligible;
	float TotalW = 0.f;
	EventTable->ForeachRow<FDFRandomEventRow>(
		TEXT("UDFRandomEventSubsystem::RollEvent"),
		[&](const FName& Key, const FDFRandomEventRow& Row)
		{
			if (CurrentFloor < Row.MinFloor)
			{
				return;
			}
			if (!Row.bCanRepeat && UsedEvents.Contains(Key))
			{
				return;
			}
			const float W = FMath::Max(0.01f, Row.Weight);
			FEntry E;
			E.RowName = Key;
			E.RowPtr = &Row;
			E.Weight = W;
			Eligible.Add(MoveTemp(E));
			TotalW += W;
		});
	if (Eligible.Num() == 0 || TotalW <= KINDA_SMALL_NUMBER)
	{
		return nullptr;
	}
	float P = FMath::FRandRange(0.f, TotalW);
	for (FEntry& E : Eligible)
	{
		P -= E.Weight;
		if (P <= 0.f)
		{
			OutRowName = E.RowName;
			return E.RowPtr;
		}
	}
	OutRowName = Eligible.Last().RowName;
	return Eligible.Last().RowPtr;
}

void UDFRandomEventSubsystem::MarkEventUsed(const FName RowName, const bool bCanRepeat)
{
	if (bCanRepeat || RowName.IsNone())
	{
		return;
	}
	UsedEvents.AddUnique(RowName);
}

void UDFRandomEventSubsystem::ExecuteChoice(
	const FDFEventChoice& Choice, ADFPlayerCharacter* const Player, FName const SourceEventRow)
{
	if (!IsValid(Player) || !Player->HasAuthority())
	{
		return;
	}
	UDFRunManager* const RM = ResolveRunManager();
	ADFPlayerState* const PS = Player->GetPlayerState<ADFPlayerState>();
	UDFAbilitySelectionSubsystem* const AbSel = ResolveAbilitySelection();

	switch (Choice.OutcomeType)
	{
	case EEventOutcomeType::None:
		break;
	case EEventOutcomeType::Heal:
		ApplyHeal(Player, Choice);
		break;
	case EEventOutcomeType::Damage:
		ApplyDamage(Player, Choice);
		break;
	case EEventOutcomeType::Gold:
		if (RM)
		{
			RM->AddRunGold(FMath::RoundToInt(Choice.OutcomeValue));
		}
		break;
	case EEventOutcomeType::AddAbility:
		if (PS && AbSel)
		{
			if (Choice.AbilityRowName.IsNone())
			{
				if (RM)
				{
					RM->TryGrantRandomAbilityByMinimumRarity(EItemRarity::Epic, PS);
				}
			}
			else
			{
				AbSel->GrantSelectedAbility(Choice.AbilityRowName, Player);
			}
			AbSel->SyncHistoryFromRun();
		}
		break;
	case EEventOutcomeType::RemoveAbility:
		if (RM && PS)
		{
			RM->RemoveOneRandomGrantedAbility(PS);
		}
		break;
	case EEventOutcomeType::AddEffect:
		if (Choice.EffectClass)
		{
			UDFGameplayEffectLibrary::ApplyEffectToSelf(Player, Choice.EffectClass, 1.f);
		}
		break;
	case EEventOutcomeType::Nothing:
		break;
	case EEventOutcomeType::RandomGood:
		ApplyRandomGood(Player);
		break;
	case EEventOutcomeType::RandomBad:
		ApplyRandomBad(Player);
		break;
	case EEventOutcomeType::AddItem:
		{
			FName ItemRow = Choice.AbilityRowName;
			if (ItemRow.IsNone() && RM && RM->ItemDataTable)
			{
				TArray<FName> Pool;
				RM->ItemDataTable->GetRowMap().GenerateKeyArray(Pool);
				if (Pool.Num() > 0)
				{
					ItemRow = Pool[FMath::RandRange(0, Pool.Num() - 1)];
				}
			}
			const int32 Qty = Choice.OutcomeValue > 0.f ? FMath::RoundToInt(Choice.OutcomeValue) : 1;
			AddItemToPlayer(Player, ItemRow, Qty);
		}
		break;
	case EEventOutcomeType::ScaleEnemyDamage:
		if (RM && Choice.OutcomeValue > 0.f)
		{
			RM->MulEnemyOutgoingDamageScale(Choice.OutcomeValue);
		}
		break;
	case EEventOutcomeType::SpawnSpecialEncounter:
		OnRequestSpecialEncounter.Broadcast(SourceEventRow, Choice.OutcomeValue, Player);
		break;
	case EEventOutcomeType::RandomHealOrDamage:
		if (FMath::FRand() < 0.5f)
		{
			ApplyHeal(Player, Choice);
		}
		else
		{
			FDFEventChoice D = Choice;
			D.OutcomeValue = Choice.SecondaryOutcomeValue;
			ApplyDamage(Player, D);
		}
		break;
	case EEventOutcomeType::SwapRandomAbility:
		if (RM && PS)
		{
			RM->RemoveOneRandomGrantedAbility(PS);
			RM->TryGrantRandomAbilityByMinimumRarity(EItemRarity::Common, PS);
		}
		if (AbSel)
		{
			AbSel->SyncHistoryFromRun();
		}
		break;
	default:
		break;
	}

	OnOutcome.Broadcast(Choice.OutcomeText);
}

bool UDFRandomEventSubsystem::DoesPlayerMeetChoiceRequirements(
	const FDFEventChoice& Choice, ADFPlayerCharacter* const Player) const
{
	UAbilitySystemComponent* const ASC = ResolvePlayerAsc(Player);
	if (!ASC)
	{
		return true;
	}
	if (Choice.RequiredTags.Num() > 0)
	{
		FGameplayTagContainer Owned;
		ASC->GetOwnedGameplayTags(Owned);
		if (!Owned.HasAll(Choice.RequiredTags))
		{
			return false;
		}
	}
	if (Choice.RequiredAgility > 0.f)
	{
		const float Agi = ASC->GetNumericAttribute(UDFAttributeSet::GetAgilityAttribute());
		if (Agi <= Choice.RequiredAgility)
		{
			return false;
		}
	}
	return true;
}

UDFRunManager* UDFRandomEventSubsystem::ResolveRunManager() const
{
	UGameInstance* const GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UDFRunManager>() : nullptr;
}

UDFAbilitySelectionSubsystem* UDFRandomEventSubsystem::ResolveAbilitySelection() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UDFAbilitySelectionSubsystem>() : nullptr;
}

UAbilitySystemComponent* UDFRandomEventSubsystem::ResolvePlayerAsc(ADFPlayerCharacter* const Player) const
{
	if (!IsValid(Player))
	{
		return nullptr;
	}
	if (UAbilitySystemComponent* const A = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Player))
	{
		return A;
	}
	if (ADFPlayerState* const PState = Player->GetPlayerState<ADFPlayerState>())
	{
		return PState->GetAbilitySystemComponent();
	}
	return nullptr;
}

void UDFRandomEventSubsystem::ApplyHeal(ADFPlayerCharacter* const Player, const FDFEventChoice& Choice) const
{
	float Amount = Choice.OutcomeValue;
	if (Choice.bOutcomeValueIsPercentOfMax)
	{
		if (UAbilitySystemComponent* const ASC = ResolvePlayerAsc(Player))
		{
			const float MaxH = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
			Amount = MaxH * (Choice.OutcomeValue * 0.01f);
		}
	}
	if (Amount > 0.f)
	{
		const FGameplayEffectSpecHandle SpecH = UDFGameplayEffectLibrary::MakeHealEffect(Amount, Player);
		if (UAbilitySystemComponent* const ASC = ResolvePlayerAsc(Player))
		{
			if (SpecH.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*SpecH.Data);
			}
		}
	}
}

void UDFRandomEventSubsystem::ApplyDamage(ADFPlayerCharacter* const Player, const FDFEventChoice& Choice) const
{
	float Amount = Choice.OutcomeValue;
	if (Choice.bOutcomeValueIsPercentOfMax)
	{
		if (UAbilitySystemComponent* const ASC = ResolvePlayerAsc(Player))
		{
			const float MaxH = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
			Amount = MaxH * (Choice.OutcomeValue * 0.01f);
		}
	}
	ApplyDamageWithAmount(Player, Amount);
}

void UDFRandomEventSubsystem::ApplyDamageWithAmount(ADFPlayerCharacter* const Player, const float RawAmount) const
{
	if (RawAmount <= 0.f)
	{
		return;
	}
	const FGameplayTag TrueTag = FDFGameplayTags::Effect_Damage_True.IsValid()
		? FDFGameplayTags::Effect_Damage_True
		: FGameplayTag::RequestGameplayTag(FName("Effect.Damage.True"), false);
	const FGameplayEffectSpecHandle SpecH = UDFGameplayEffectLibrary::MakeDamageEffect(RawAmount, TrueTag, Player);
	if (UAbilitySystemComponent* const ASC = ResolvePlayerAsc(Player))
	{
		if (SpecH.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*SpecH.Data);
		}
	}
}

void UDFRandomEventSubsystem::ApplyRandomGood(ADFPlayerCharacter* const Player) const
{
	UDFRunManager* const RM = ResolveRunManager();
	if (FMath::FRand() < 0.5f)
	{
		if (UAbilitySystemComponent* const ASC = ResolvePlayerAsc(Player))
		{
			const float MaxH = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
			const FGameplayEffectSpecHandle SpecH = UDFGameplayEffectLibrary::MakeHealEffect(MaxH * 0.15f, Player);
			if (SpecH.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*SpecH.Data);
			}
		}
	}
	else
	{
		if (RM)
		{
			RM->AddRunGold(15);
		}
	}
}

void UDFRandomEventSubsystem::ApplyRandomBad(ADFPlayerCharacter* const Player) const
{
	UDFRunManager* const RM = ResolveRunManager();
	if (FMath::FRand() < 0.5f)
	{
		ApplyDamageWithAmount(Player, 10.f);
	}
	else
	{
		if (RM)
		{
			RM->SpendGold(5);
		}
	}
}

void UDFRandomEventSubsystem::AddItemToPlayer(ADFPlayerCharacter* const Player, FName const ItemRow, int32 Quantity) const
{
	if (ItemRow.IsNone() || Quantity <= 0)
	{
		return;
	}
	UDFRunManager* const RM = ResolveRunManager();
	UDFInventoryComponent* const Inv = Player
		? Player->FindComponentByClass<UDFInventoryComponent>()
		: nullptr;
	if (Inv)
	{
		if (RM && RM->ItemDataTable)
		{
			Inv->ItemDataTable = RM->ItemDataTable;
		}
		Inv->AddItem(ItemRow, Quantity);
	}
}
