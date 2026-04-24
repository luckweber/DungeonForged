// Source/DungeonForged/Private/Characters/ADFPlayerState.cpp

#include "Characters/ADFPlayerState.h"
#include "Progression/UDFLevelingComponent.h"
#include "ADFDungeonManager.h"
#include "AbilitySystemComponent.h"
#include "Characters/ADFPlayerCharacter.h"
#include "GAS/UDFAttributeSet.h"
#include "Engine/DataTable.h"
#include "Events/UDFRandomEventSubsystem.h"
#include "UI/UDFAbilitySelectionWidget.h"
#include "UI/UDFAbilitySelectionSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogDFPlayerState, Log, All);

ADFPlayerState::ADFPlayerState()
{
	const bool bIsCDO = HasAnyFlags(RF_ClassDefaultObject);
	UE_LOG(LogDFPlayerState, Verbose, TEXT("Ctor %s %s (outer=%s)"),
		bIsCDO ? TEXT("[CDO]") : TEXT("[Instance]"), *GetName(), GetOuter() ? *GetOuter()->GetName() : TEXT("null"));

	// Replication tick rate (was SetNetUpdateFrequency; direct member works with all include orders / IWYU)
	NetUpdateFrequency = 100.f;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UDFAttributeSet>(TEXT("AttributeSet"));
	LevelingComponent = CreateDefaultSubobject<UDFLevelingComponent>(TEXT("LevelingComponent"));

	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

UAbilitySystemComponent* ADFPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ADFPlayerState::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogDFPlayerState, Log, TEXT("BeginPlay %s | NetMode=%d HasAuth=%d PlayerId=%d"),
		*GetName(), GetWorld() ? (int32)GetWorld()->GetNetMode() : -1, HasAuthority() ? 1 : 0, GetPlayerId());
}

void ADFPlayerState::GrantAbilitiesFromDataTable(UDataTable* AbilityTable)
{
	if (!HasAuthority() || !AbilitySystemComponent || !AbilityTable)
	{
		UE_LOG(LogDFPlayerState, Verbose, TEXT("GrantAbilitiesFromDataTable: skip (Auth=%d ASC=%d Table=%d) %s"),
			HasAuthority() ? 1 : 0, AbilitySystemComponent != nullptr, AbilityTable != nullptr, *GetName());
		return;
	}

	UE_LOG(LogDFPlayerState, Log, TEXT("GrantAbilitiesFromDataTable: %s from table %s"),
		*GetName(), *AbilityTable->GetName());

	int32 GrantedCount = 0;
	static const FString Ctx(TEXT("ADFPlayerState::GrantAbilitiesFromDataTable"));
	AbilityTable->ForeachRow<FDFAbilityTableRow>(Ctx, [this, &GrantedCount](const FName& RowKey, const FDFAbilityTableRow& Row) {
		if (!Row.AbilityClass)
		{
			return;
		}
		const FGameplayAbilitySpec Spec(Row.AbilityClass, Row.AbilityLevel, INDEX_NONE, this);
		AbilitySystemComponent->GiveAbility(Spec);
		++GrantedCount;
	});
	UE_LOG(LogDFPlayerState, Log, TEXT("GrantAbilitiesFromDataTable: granted %d ability specs for %s"), GrantedCount, *GetName());
}

void ADFPlayerState::InitializeAttributesFromDataTable(UDataTable* AttributeTable, FName RowName)
{
	if (!HasAuthority() || !AbilitySystemComponent || !AttributeTable || RowName.IsNone())
	{
		UE_LOG(LogDFPlayerState, Verbose, TEXT("InitializeAttributesFromDataTable: skip (Auth=%d ASC=%d Table=%d RowNone=%d) %s | Row=%s"),
			HasAuthority() ? 1 : 0, AbilitySystemComponent != nullptr, AttributeTable != nullptr, RowName.IsNone() ? 1 : 0, *GetName(), *RowName.ToString());
		return;
	}
	if (!AbilitySystemComponent->GetAvatarActor())
	{
		UE_LOG(LogDFPlayerState, Verbose, TEXT("InitializeAttributesFromDataTable: no avatar yet (deferred) %s | Row=%s"), *GetName(), *RowName.ToString());
		// Call after InitAbilityActorInfo on the owning pawn (e.g. from GameMode after possess).
		return;
	}

	static const FString Ctx(TEXT("ADFPlayerState::InitializeAttributesFromDataTable"));
	const FDFAttributeInitTableRow* Row = AttributeTable->FindRow<FDFAttributeInitTableRow>(RowName, Ctx, false);
	if (!Row || !Row->StartupGameplayEffect)
	{
		UE_LOG(LogDFPlayerState, Warning, TEXT("InitializeAttributesFromDataTable: bad row or empty StartupGE %s | Row=%s"), *GetName(), *RowName.ToString());
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle =
		AbilitySystemComponent->MakeOutgoingSpec(Row->StartupGameplayEffect, 1.f, EffectContext);
	if (SpecHandle.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		UE_LOG(LogDFPlayerState, Log, TEXT("InitializeAttributesFromDataTable: applied startup GE %s for %s | Row=%s"),
			*GetNameSafe(Row->StartupGameplayEffect.Get()), *GetName(), *RowName.ToString());
	}
	else
	{
		UE_LOG(LogDFPlayerState, Warning, TEXT("InitializeAttributesFromDataTable: MakeOutgoingSpec failed %s | Row=%s"), *GetName(), *RowName.ToString());
	}
}

void ADFPlayerState::Client_OpenAbilitySelectionScreen_Implementation(
	int32 const FloorCleared, const TArray<FDFAbilityRolledChoice>& OfferChoices, int32 const SkipGold, int32 const OfferId, float const OptionalTimerSeconds)
{
	if (OfferChoices.Num() == 0)
	{
		return;
	}
	APlayerController* const PC = GetPlayerController();
	UWorld* const W = GetWorld();
	if (!PC || !W)
	{
		return;
	}
	UDFAbilitySelectionSubsystem* const Sub = W->GetSubsystem<UDFAbilitySelectionSubsystem>();
	if (!Sub || !Sub->SelectionWidgetClass)
	{
		UE_LOG(LogDFPlayerState, Warning, TEXT("Client_OpenAbilitySelectionScreen: set SelectionWidgetClass on UDFAbilitySelectionSubsystem (or CDO)."));
		return;
	}
	UDFAbilitySelectionWidget* const Screen = CreateWidget<UDFAbilitySelectionWidget>(PC, Sub->SelectionWidgetClass);
	if (!Screen)
	{
		return;
	}
	Screen->AddToViewport(10);
	Screen->InitializeOffer(this, FloorCleared, SkipGold, OfferId, OfferChoices, OptionalTimerSeconds);
	UGameplayStatics::SetGlobalTimeDilation(W, 0.0001f);
	PC->SetShowMouseCursor(true);
	FInputModeUIOnly Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(Mode);
}

void ADFPlayerState::Client_ResumeAfterAbilitySelection_Implementation()
{
	if (UWorld* const W = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(W, 1.f);
		if (UDFAbilitySelectionSubsystem* const Sub = W->GetSubsystem<UDFAbilitySelectionSubsystem>())
		{
			Sub->CloseActiveSelectionWidget();
		}
	}
	if (APlayerController* const PC = GetPlayerController())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void ADFPlayerState::Server_FinishAbilitySelection_Implementation(int32 const OfferId, bool const bSkipped, FName const SelectedRowName)
{
	if (!HasAuthority())
	{
		return;
	}
	UGameInstance* const GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	UDFDungeonManager* const DM = GI->GetSubsystem<UDFDungeonManager>();
	if (!DM)
	{
		return;
	}
	if (DM->bFloorOfferResolved || OfferId != DM->ActiveFloorOfferId)
	{
		Client_ResumeAfterAbilitySelection();
		return;
	}
	DM->bFloorOfferResolved = true;
	if (UWorld* const W = GetWorld())
	{
		if (UDFAbilitySelectionSubsystem* const Sub = W->GetSubsystem<UDFAbilitySelectionSubsystem>())
		{
			ADFPlayerCharacter* const P = GetPawn<ADFPlayerCharacter>();
			if (bSkipped)
			{
				Sub->SkipSelection(P);
			}
			else if (!SelectedRowName.IsNone() && P)
			{
				Sub->GrantSelectedAbility(SelectedRowName, P);
			}
		}
	}
	DM->AdvanceToNextFloor();
	if (UWorld* const W2 = GetWorld())
	{
		for (FConstPlayerControllerIterator It = W2->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* const PCG = It->Get())
			{
				if (ADFPlayerState* const Ops = PCG->GetPlayerState<ADFPlayerState>())
				{
					Ops->Client_ResumeAfterAbilitySelection();
				}
			}
		}
	}
}

void ADFPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADFPlayerState, ReplicatedRunGold);
}

void ADFPlayerState::AuthoritySetReplicatedRunGold(int32 const NewTotal)
{
	if (!HasAuthority())
	{
		return;
	}
	ReplicatedRunGold = FMath::Max(0, NewTotal);
	UE_LOG(LogDFPlayerState, Verbose, TEXT("AuthoritySetReplicatedRunGold %s | %d"), *GetName(), ReplicatedRunGold);
	OnReplicatedRunGold.Broadcast(ReplicatedRunGold);
}

void ADFPlayerState::OnRep_ReplicatedRunGold()
{
	UE_LOG(LogDFPlayerState, Verbose, TEXT("OnRep_ReplicatedRunGold %s | Gold=%d"), *GetName(), ReplicatedRunGold);
	OnReplicatedRunGold.Broadcast(ReplicatedRunGold);
}

void ADFPlayerState::Server_ExecuteRandomEventChoice_Implementation(FDFEventChoice Choice, FName EventRowName)
{
	ADFPlayerCharacter* const Ch = GetPawn() ? Cast<ADFPlayerCharacter>(GetPawn()) : nullptr;
	if (!IsValid(Ch) || !GetWorld())
	{
		return;
	}
	if (UDFRandomEventSubsystem* const Ev = GetWorld()->GetSubsystem<UDFRandomEventSubsystem>())
	{
		Ev->ExecuteChoice(Choice, Ch, EventRowName);
		if (UDataTable* const DT = Ev->EventTable)
		{
			if (const FDFRandomEventRow* const R = DT->FindRow<FDFRandomEventRow>(EventRowName, TEXT("Server_ExecuteRandomEventChoice"), false))
			{
				Ev->MarkEventUsed(EventRowName, R->bCanRepeat);
			}
		}
	}
}

bool ADFPlayerState::Server_ExecuteRandomEventChoice_Validate(FDFEventChoice /*Choice*/, FName /*EventRowName*/)
{
	return true;
}
