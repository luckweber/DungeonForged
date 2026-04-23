// Source/DungeonForged/Private/Characters/ADFPlayerState.cpp

#include "Characters/ADFPlayerState.h"
#include "ADFDungeonManager.h"
#include "AbilitySystemComponent.h"
#include "Characters/ADFPlayerCharacter.h"
#include "GAS/UDFAttributeSet.h"
#include "UI/UDFAbilitySelectionWidget.h"
#include "UI/UDFAbilitySelectionSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ADFPlayerState::ADFPlayerState()
{
	// Replication tick rate (was SetNetUpdateFrequency; direct member works with all include orders / IWYU)
	NetUpdateFrequency = 100.f;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UDFAttributeSet>(TEXT("AttributeSet"));

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
}

void ADFPlayerState::GrantAbilitiesFromDataTable(UDataTable* AbilityTable)
{
	if (!HasAuthority() || !AbilitySystemComponent || !AbilityTable)
	{
		return;
	}

	static const FString Ctx(TEXT("ADFPlayerState::GrantAbilitiesFromDataTable"));
	AbilityTable->ForeachRow<FDFAbilityTableRow>(Ctx, [this](const FName& RowKey, const FDFAbilityTableRow& Row) {
		if (!Row.AbilityClass)
		{
			return;
		}
		const FGameplayAbilitySpec Spec(Row.AbilityClass, Row.AbilityLevel, INDEX_NONE, this);
		AbilitySystemComponent->GiveAbility(Spec);
	});
}

void ADFPlayerState::InitializeAttributesFromDataTable(UDataTable* AttributeTable, FName RowName)
{
	if (!HasAuthority() || !AbilitySystemComponent || !AttributeTable || RowName.IsNone())
	{
		return;
	}

	static const FString Ctx(TEXT("ADFPlayerState::InitializeAttributesFromDataTable"));
	const FDFAttributeInitTableRow* Row = AttributeTable->FindRow<FDFAttributeInitTableRow>(RowName, Ctx, false);
	if (!Row || !Row->StartupGameplayEffect)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle =
		AbilitySystemComponent->MakeOutgoingSpec(Row->StartupGameplayEffect, 1.f, EffectContext);
	if (SpecHandle.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
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
		UE_LOG(LogTemp, Warning, TEXT("Client_OpenAbilitySelectionScreen: set SelectionWidgetClass on UDFAbilitySelectionSubsystem (or CDO)."));
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
