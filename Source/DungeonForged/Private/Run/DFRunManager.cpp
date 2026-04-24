// Source/DungeonForged/Private/Run/DFRunManager.cpp

#include "Run/DFRunManager.h"
#include "Run/DFSaveGame.h"
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

void UDFRunManager::SetPendingRunArrival(const EDFRunTravelReason InReason, const FName InClassForNewRun)
{
	PendingArrivalReason = InReason;
	PendingClassForArrival = (InReason == EDFRunTravelReason::NewRun) ? InClassForNewRun : NAME_None;
}

void UDFRunManager::ClearRunArrivalContext()
{
	PendingArrivalReason = EDFRunTravelReason::None;
	PendingClassForArrival = NAME_None;
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
}

void UDFRunManager::RestoreRunState(ADFPlayerCharacter* const Player)
{
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

	if (UDFSaveGame* const Meta = UDFSaveGame::Load())
	{
		++Meta->TotalRuns;
		UDFSaveGame::Save(Meta);
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
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (World && World->GetNetMode() == NM_Client)
	{
		return;
	}

	bRunInProgress = false;

	const int32 FinalScore = CalculateFinalScore();

	if (UDFSaveGame* const Meta = UDFSaveGame::Load())
	{
		++Meta->TotalWins;
		AddUniqueName(Meta->UnlockedClasses, RunState.SelectedClass);
		for (const FName Ability : RunState.GrantedAbilities)
		{
			AddUniqueName(Meta->UnlockedAbilities, Ability);
		}
		if (FinalScore > Meta->HighScore)
		{
			Meta->HighScore = FinalScore;
		}
		UDFSaveGame::Save(Meta);
	}

	OnRunEndedSuccessfully.Broadcast(FinalScore);
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
