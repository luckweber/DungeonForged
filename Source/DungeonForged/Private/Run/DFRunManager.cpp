// Source/DungeonForged/Private/Run/DFRunManager.cpp

#include "Run/DFRunManager.h"
#include "Run/DFSaveGame.h"
#include "World/DFWorldTypes.h"
#include "GameModes/Nexus/DFNexusTypes.h"
#include "ADFDungeonManager.h"
#include "GameModes/Run/ADFRunGameState.h"
#include "Events/UDFRandomEventSubsystem.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Characters/ADFPlayerState.h"
#include "DFInventoryComponent.h"
#include "GAS/UDFAttributeSet.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "AbilitySystemComponent.h"
#include "HAL/PlatformTime.h"
DEFINE_LOG_CATEGORY_STATIC(LogDFRun, Log, All);

void UDFRunManager::Deinitialize()
{
	if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		World->GetTimerManager().ClearTimer(DeathScreenTimerHandle);
	}
	Super::Deinitialize();
}

void UDFRunManager::SetNexusArrivalReason(const ERunNexusTravelReason InReason)
{
	bNexusArrivalSet = true;
	LastNexusArrivalReason = InReason;
}

ERunNexusTravelReason UDFRunManager::GetNexusArrivalReason() const
{
	return bNexusArrivalSet ? LastNexusArrivalReason : ERunNexusTravelReason::FirstLaunch;
}

void UDFRunManager::ClearNexusArrivalContext()
{
	bNexusArrivalSet = false;
}

void UDFRunManager::SetPendingRunArrival(const EDFRunTravelReason InReason, const FName InClassForNewRun)
{
	PendingArrivalReason = InReason;
	PendingClassForArrival = (InReason == EDFRunTravelReason::NewRun) ? InClassForNewRun : NAME_None;
	switch (InReason)
	{
	case EDFRunTravelReason::NewRun: LastTravelReason = ETravelReason::NewRun; break;
	case EDFRunTravelReason::NextFloor: LastTravelReason = ETravelReason::NextFloor; break;
	default: LastTravelReason = ETravelReason::None; break;
	}
}

void UDFRunManager::SetPendingWorldTravel(const ETravelReason WorldReason, const FName ClassForNewRun)
{
	LastTravelReason = WorldReason;
	switch (WorldReason)
	{
	case ETravelReason::NewRun:
		SetPendingRunArrival(EDFRunTravelReason::NewRun, ClassForNewRun);
		break;
	case ETravelReason::NextFloor:
		SetPendingRunArrival(EDFRunTravelReason::NextFloor, NAME_None);
		break;
	default:
		PendingClassForArrival = NAME_None;
		PendingArrivalReason = EDFRunTravelReason::None;
		break;
	}
}

void UDFRunManager::ClearRunArrivalContext()
{
	PendingArrivalReason = EDFRunTravelReason::None;
	PendingClassForArrival = NAME_None;
	LastTravelReason = ETravelReason::None;
}

void UDFRunManager::CaptureRunState()
{
	UWorld* const W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!W || W->GetNetMode() == NM_Client)
	{
		return;
	}
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFDungeonManager* const DM = GI->GetSubsystem<UDFDungeonManager>())
		{
			RunState.CurrentFloor = FMath::Max(1, DM->CurrentFloor);
		}
	}
	RunState.AbilityHistory = RunState.GrantedAbilities;
	if (APlayerController* const PC = W->GetFirstPlayerController())
	{
		if (const ADFPlayerCharacter* const P = PC->GetPawn<ADFPlayerCharacter>())
		{
			if (const ADFPlayerState* const PS = P->GetPlayerState<ADFPlayerState>())
			{
				if (UAbilitySystemComponent* const ASC = PS->GetAbilitySystemComponent())
				{
					const float Mh = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
					const float Ch = ASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute());
					RunState.HealthPercent = (Mh > KINDA_SMALL_NUMBER) ? (Ch / Mh) : -1.f;
					const float Mm = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxManaAttribute());
					const float Cm = ASC->GetNumericAttribute(UDFAttributeSet::GetManaAttribute());
					RunState.ManaPercent = (Mm > KINDA_SMALL_NUMBER) ? (Cm / Mm) : -1.f;
					RunState.RunCharacterLevel = FMath::Max(
						1, FMath::RoundToInt(ASC->GetNumericAttribute(UDFAttributeSet::GetCharacterLevelAttribute())));
				}
			}
		}
	}
}

void UDFRunManager::RestoreRunState(ADFPlayerCharacter* const Player)
{
	if (GetTravelArrivalReason() == ETravelReason::NewRun)
	{
		return;
	}
	ApplyRunStateToPlayer(Player);
}

const FDFClassTableRow* UDFRunManager::FindClassTableRow(const FName ClassRowName) const
{
	return FindClassRow(ClassRowName);
}

void UDFRunManager::StartNewRun(FName ClassName)
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (World && World->GetNetMode() == NM_Client)
	{
		return;
	}

	const FDFClassTableRow* const ClassDef = FindClassRow(ClassName);
	if (!ClassDef)
	{
		UE_LOG(LogDFRun, Warning, TEXT("StartNewRun: missing class row %s in ClassDataTable"), *ClassName.ToString());
		return;
	}

	bEndRunPersistenceApplied = false;
	RunState = FDFRunState();
	RunState.CurrentFloor = 1;
	RunState.SelectedClass = ClassName;
	RunState.EquippedItems.Reset();
	RunState.GrantedAbilities = ClassDef->StartingAbilities;
	RunState.Gold = 0;
	RunState.Score = 0;
	RunState.EnemyOutgoingDamageScale = 1.f;
	RunState.RunStartTime = static_cast<float>(FPlatformTime::Seconds());
	bRunInProgress = true;
	SyncReplicatedRunGoldToPlayerStates();
	OnGoldChanged.Broadcast(RunState.Gold);

	if (UWorld* const W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		if (UDFRandomEventSubsystem* const Ev = W->GetSubsystem<UDFRandomEventSubsystem>())
		{
			Ev->ResetUsedEvents();
		}
	}

	if (World)
	{
		if (APlayerController* const PC = World->GetFirstPlayerController())
		{
			if (ADFPlayerCharacter* const Pawn = PC->GetPawn<ADFPlayerCharacter>())
			{
				ApplyRunStateToPlayer(Pawn);
			}
		}
	}
}

void UDFRunManager::OnPlayerDied(bool const bQueueDefaultDeathScreen)
{
	if (!bRunInProgress)
	{
		return;
	}
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (World && World->GetNetMode() == NM_Client)
	{
		return;
	}

	bRunInProgress = false;

	if (UDFSaveGame* const Meta = UDFSaveGame::Load())
	{
		if (RunState.Score > Meta->HighScore)
		{
			Meta->HighScore = RunState.Score;
		}
		UDFSaveGame::Save(Meta);
	}

	OnRunFailed.Broadcast();

	if (World && bQueueDefaultDeathScreen)
	{
		World->GetTimerManager().SetTimer(
			DeathScreenTimerHandle,
			this,
			&UDFRunManager::ShowDeathScreenCallback,
			FMath::Max(0.01f, DeathScreenDelaySeconds),
			false);
	}
}

void UDFRunManager::ShowDeathScreenCallback()
{
	OnShowDeathScreen.Broadcast();
}

void UDFRunManager::OnRunCompleted()
{
	if (!bRunInProgress)
	{
		return;
	}
	UWorld* const World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (World && World->GetNetMode() == NM_Client)
	{
		return;
	}
	FDFRunSummary S;
	if (ADFRunGameState* const RGS = World ? World->GetGameState<ADFRunGameState>() : nullptr)
	{
		S = RGS->GetRunSummary();
	}
	else
	{
		S.FloorReached = RunState.CurrentFloor;
		S.Kills = 0;
		S.Gold = RunState.Gold;
		const float Elapsed = static_cast<float>(FPlatformTime::Seconds()) - RunState.RunStartTime;
		S.TimeSeconds = FMath::Max(0.f, Elapsed);
		S.ClassName = RunState.SelectedClass;
		S.AbilitiesCollected = RunState.GrantedAbilities;
	}
	ApplyEndOfRunPersistence(ETravelReason::Victory, S);
}

void UDFRunManager::ApplyEndOfRunPersistence(const ETravelReason Why, FDFRunSummary const& S)
{
	UWorld* const World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (World && World->GetNetMode() == NM_Client)
	{
		return;
	}
	if (bEndRunPersistenceApplied)
	{
		return;
	}
	if (Why != ETravelReason::Victory && Why != ETravelReason::Defeat && Why != ETravelReason::AbandonRun)
	{
		return;
	}
	bEndRunPersistenceApplied = true;
	bRunInProgress = false;
	int32 XpGain = 0;
	switch (Why)
	{
	case ETravelReason::Victory:
		XpGain = 500 + S.FloorReached * 50 + S.Kills * 2;
		break;
	case ETravelReason::Defeat:
		XpGain = 100 + S.FloorReached * 20 + S.Kills * 1;
		break;
	case ETravelReason::AbandonRun:
		XpGain = 25 + S.FloorReached * 5;
		break;
	default:
		return;
	}
	UDFSaveGame* const Meta = UDFSaveGame::Load();
	if (!Meta)
	{
		return;
	}
	++Meta->TotalRuns;
	Meta->MetaXP += XpGain;
	Meta->TotalPlayTimeSeconds += S.TimeSeconds;
	Meta->LifetimeKills += S.Kills;
	if (S.FloorReached > Meta->BestFloorReached)
	{
		Meta->BestFloorReached = S.FloorReached;
	}
	if (S.Kills > Meta->BestKillsInRun)
	{
		Meta->BestKillsInRun = S.Kills;
	}
	if (Why == ETravelReason::Victory)
	{
		++Meta->TotalWins;
		AddUniqueName(Meta->UnlockedClasses, S.ClassName);
		for (const FName Ability : S.AbilitiesCollected)
		{
			AddUniqueName(Meta->UnlockedAbilities, Ability);
		}
		const int32 FinalScore = CalculateFinalScore();
		if (FinalScore > Meta->HighScore)
		{
			Meta->HighScore = FinalScore;
		}
		// Data-driven gating: example unlock when the run was deep enough (designer can replace).
		if (S.FloorReached >= 3)
		{
			FDFPendingUnlockEntry E;
			E.Type = ENexusPendingUnlockType::UnlockClass;
			E.ClassRow = S.ClassName;
			Meta->PendingUnlocks.Add(E);
		}
		OnRunEndedSuccessfully.Broadcast(FinalScore);
	}
	UDFSaveGame::Save(Meta);
}

int32 UDFRunManager::CalculateFinalScore() const
{
	const float Elapsed = static_cast<float>(FPlatformTime::Seconds()) - RunState.RunStartTime;
	const int32 TimeBonus = FMath::Max(0, FMath::RoundToInt(600.f - Elapsed) * 2);
	return RunState.Score + RunState.Gold * 5 + RunState.CurrentFloor * 100 + TimeBonus;
}

void UDFRunManager::AddAbilityReward(const FName AbilityRow)
{
	if (AbilityRow.IsNone())
	{
		return;
	}
	if (!bRunInProgress)
	{
		return;
	}
	AddUniqueName(RunState.GrantedAbilities, AbilityRow);
}

void UDFRunManager::GetRandomAbilityOfferCandidates(const int32 InCount, TArray<FName>& OutRowNames) const
{
	OutRowNames.Reset();
	if (!AbilityDataTable || InCount <= 0)
	{
		return;
	}
	TArray<FName> Pool;
	AbilityDataTable->GetRowMap().GenerateKeyArray(Pool);
	for (int32 i = Pool.Num() - 1; i >= 0; --i)
	{
		if (RunState.GrantedAbilities.Contains(Pool[i]))
		{
			Pool.RemoveAt(i);
		}
	}
	for (int32 i = Pool.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		Pool.Swap(i, j);
	}
	const int32 N = FMath::Min(InCount, Pool.Num());
	for (int32 i = 0; i < N; ++i)
	{
		OutRowNames.Add(Pool[i]);
	}
}

void UDFRunManager::AdvanceFloor(const int32 FloorDelta)
{
	if (!bRunInProgress || FloorDelta == 0)
	{
		return;
	}
	RunState.CurrentFloor = FMath::Max(1, RunState.CurrentFloor + FloorDelta);
	OnRunFloorChanged.Broadcast(RunState.CurrentFloor);
}

void UDFRunManager::AddRunGold(const int32 Delta)
{
	if (!bRunInProgress)
	{
		return;
	}
	const int32 OldGold = RunState.Gold;
	RunState.Gold = FMath::Max(0, RunState.Gold + Delta);
	const int32 GoldDelta = RunState.Gold - OldGold;
	if (GoldDelta != 0)
	{
		if (UWorld* const W2 = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
		{
			if (ADFRunGameState* const RGS = W2->GetGameState<ADFRunGameState>())
			{
				RGS->AddGold(GoldDelta);
			}
		}
	}
	SyncReplicatedRunGoldToPlayerStates();
	OnGoldChanged.Broadcast(RunState.Gold);
}

bool UDFRunManager::SpendGold(const int32 Amount)
{
	if (!bRunInProgress || Amount < 0)
	{
		return false;
	}
	if (RunState.Gold < Amount)
	{
		return false;
	}
	RunState.Gold -= Amount;
	SyncReplicatedRunGoldToPlayerStates();
	OnGoldChanged.Broadcast(RunState.Gold);
	return true;
}

void UDFRunManager::MulEnemyOutgoingDamageScale(const float Mult)
{
	if (!bRunInProgress || Mult <= 0.f)
	{
		return;
	}
	RunState.EnemyOutgoingDamageScale = FMath::Max(0.1f, RunState.EnemyOutgoingDamageScale * Mult);
}

void UDFRunManager::SyncReplicatedRunGoldToPlayerStates() const
{
	UWorld* const W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	AGameStateBase* const GS = W ? W->GetGameState() : nullptr;
	if (!GS)
	{
		return;
	}
	for (APlayerState* const PS : GS->PlayerArray)
	{
		if (ADFPlayerState* const DPS = Cast<ADFPlayerState>(PS))
		{
			DPS->AuthoritySetReplicatedRunGold(RunState.Gold);
		}
	}
}

void UDFRunManager::AddRunScore(const int32 Delta)
{
	if (!bRunInProgress)
	{
		return;
	}
	RunState.Score = FMath::Max(0, RunState.Score + Delta);
}

void UDFRunManager::AddUniqueName(TArray<FName>& ToArray, const FName Name) const
{
	if (Name.IsNone())
	{
		return;
	}
	ToArray.AddUnique(Name);
}

const FDFClassTableRow* UDFRunManager::FindClassRow(const FName ClassName) const
{
	if (!ClassDataTable || ClassName.IsNone())
	{
		return nullptr;
	}
	return ClassDataTable->FindRow<FDFClassTableRow>(ClassName, TEXT("UDFRunManager::FindClassRow"), false);
}

void UDFRunManager::ApplyClassToAttributes(UAbilitySystemComponent* ASC, const FDFClassTableRow& ClassRow) const
{
	if (!ASC)
	{
		return;
	}
	for (const TPair<FGameplayAttribute, float>& Pair : ClassRow.BaseAttributeValues)
	{
		if (Pair.Key.IsValid())
		{
			ASC->SetNumericAttributeBase(Pair.Key, Pair.Value);
		}
	}
}

void UDFRunManager::GrantAbilitiesForCurrentRun(ADFPlayerState* PlayerState)
{
	if (!PlayerState || !AbilityDataTable)
	{
		return;
	}
	UAbilitySystemComponent* const ASC = PlayerState->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	ASC->ClearAllAbilities();

	for (int32 Idx = 0; Idx < RunState.GrantedAbilities.Num(); ++Idx)
	{
		const FName AbilityName = RunState.GrantedAbilities[Idx];
		if (AbilityName.IsNone())
		{
			continue;
		}
		const FDFAbilityTableRow* const Row = AbilityDataTable->FindRow<FDFAbilityTableRow>(AbilityName, TEXT("UDFRunManager::GrantAbilitiesForCurrentRun"), false);
		if (!Row || !Row->AbilityClass)
		{
			UE_LOG(LogDFRun, Warning, TEXT("GrantAbilitiesForCurrentRun: missing or invalid row %s"), *AbilityName.ToString());
			continue;
		}
		const int32 InId = FMath::Min(Idx, 3);
		const FGameplayAbilitySpec Spec(Row->AbilityClass, Row->AbilityLevel, InId, PlayerState);
		ASC->GiveAbility(Spec);
	}
}

void UDFRunManager::RemoveOneRandomGrantedAbility(ADFPlayerState* PlayerState)
{
	if (!bRunInProgress || RunState.GrantedAbilities.Num() == 0)
	{
		return;
	}
	const int32 Idx = FMath::RandRange(0, RunState.GrantedAbilities.Num() - 1);
	RunState.GrantedAbilities.RemoveAt(Idx);
	GrantAbilitiesForCurrentRun(PlayerState);
}

bool UDFRunManager::TryGrantRandomAbilityByMinimumRarity(const EItemRarity MinRarity, ADFPlayerState* PlayerState)
{
	if (!bRunInProgress || !AbilityDataTable || !PlayerState)
	{
		return false;
	}
	const uint8 MinU = static_cast<uint8>(MinRarity);
	TArray<FName> Pool;
	AbilityDataTable->ForeachRow<FDFAbilityTableRow>(
		TEXT("TryGrantRandomAbilityByMinimumRarity"),
		[&](const FName& Key, const FDFAbilityTableRow& Row)
		{
			if (static_cast<uint8>(Row.Rarity) < MinU)
			{
				return;
			}
			if (!RunState.GrantedAbilities.Contains(Key))
			{
				Pool.Add(Key);
			}
		});
	if (Pool.Num() == 0)
	{
		return false;
	}
	for (int32 I = Pool.Num() - 1; I > 0; --I)
	{
		const int32 J = FMath::RandRange(0, I);
		Pool.Swap(I, J);
	}
	AddAbilityReward(Pool[0]);
	GrantAbilitiesForCurrentRun(PlayerState);
	return true;
}

void UDFRunManager::RestoreInventoryFromRunState(UDFInventoryComponent* Inv) const
{
	if (!Inv)
	{
		return;
	}
	if (!ItemDataTable)
	{
		return;
	}
	Inv->ItemDataTable = ItemDataTable;

	// Rebuild: remove everything first (authority path).
	if (Inv->GetOwner() && Inv->GetOwner()->HasAuthority())
	{
		TArray<FDFInventorySlot> Snapshot = Inv->Items;
		for (const FDFInventorySlot& Slot : Snapshot)
		{
			if (!Slot.RowName.IsNone() && Slot.Quantity > 0)
			{
				Inv->RemoveItem(Slot.RowName, Slot.Quantity);
			}
		}
	}

	for (const FName ItemRow : RunState.EquippedItems)
	{
		if (ItemRow.IsNone())
		{
			continue;
		}
		Inv->AddItem(ItemRow, 1);
	}
	for (int32 Idx = 0; Idx < Inv->Items.Num(); ++Idx)
	{
		if (RunState.EquippedItems.Contains(Inv->Items[Idx].RowName))
		{
			Inv->EquipItem(Idx);
		}
	}
}

void UDFRunManager::ApplyRunStateToPlayer(ADFPlayerCharacter* Player)
{
	if (!Player || !Player->GetWorld())
	{
		return;
	}
	if (!Player->HasAuthority())
	{
		return;
	}

	ADFPlayerState* const PlayerState = Player->GetPlayerState<ADFPlayerState>();
	if (!PlayerState)
	{
		UE_LOG(LogDFRun, Warning, TEXT("ApplyRunStateToPlayer: no PlayerState yet."));
		return;
	}

	const FDFClassTableRow* const ClassRow = FindClassRow(RunState.SelectedClass);
	if (UAbilitySystemComponent* const ASC = PlayerState->GetAbilitySystemComponent())
	{
		ASC->InitAbilityActorInfo(PlayerState, Player);
		if (ClassRow)
		{
			ApplyClassToAttributes(ASC, *ClassRow);
		}
		GrantAbilitiesForCurrentRun(PlayerState);
	}
	if (ClassRow && ClassRow->CharacterMesh)
	{
		if (USkeletalMeshComponent* const Mesh = Player->GetMesh())
		{
			Mesh->SetSkeletalMesh(ClassRow->CharacterMesh);
		}
	}

	if (UAbilitySystemComponent* const ASC2 = PlayerState->GetAbilitySystemComponent())
	{
		if (RunState.HealthPercent >= 0.f && RunState.HealthPercent <= 1.f)
		{
			const float Mh2 = ASC2->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
			ASC2->SetNumericAttributeBase(UDFAttributeSet::GetHealthAttribute(), Mh2 * RunState.HealthPercent);
		}
		if (RunState.ManaPercent >= 0.f && RunState.ManaPercent <= 1.f)
		{
			const float Mm2 = ASC2->GetNumericAttribute(UDFAttributeSet::GetMaxManaAttribute());
			ASC2->SetNumericAttributeBase(UDFAttributeSet::GetManaAttribute(), Mm2 * RunState.ManaPercent);
		}
	}

	UDFInventoryComponent* const Inv = Player->FindComponentByClass<UDFInventoryComponent>();
	if (Inv)
	{
		RestoreInventoryFromRunState(Inv);
	}
	else if (PlayerState)
	{
		if (UDFInventoryComponent* const InvPS = PlayerState->FindComponentByClass<UDFInventoryComponent>())
		{
			RestoreInventoryFromRunState(InvPS);
		}
	}
}
