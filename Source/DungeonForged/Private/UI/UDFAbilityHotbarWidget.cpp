// Source/DungeonForged/Private/UI/UDFAbilityHotbarWidget.cpp
#include "UI/UDFAbilityHotbarWidget.h"
#include "AbilitySystemComponent.h"
#include "Characters/ADFPlayerCharacter.h"
#include "UI/DFAbilityBarTypes.h"
#include "Components/ProgressBar.h"
#include "Data/DFDataTableStructs.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "GAS/UDFAttributeSet.h"
#include "Run/DFRunManager.h"
#include "UI/UDFAbilitySlotWidget.h"

namespace
{
	void SetProgressBarPercent(UProgressBar* const Bar, const float Current, const float MaxValue)
	{
		if (!Bar)
		{
			return;
		}
		const float SafeMax = FMath::Max(MaxValue, 0.f);
		const float Percent = SafeMax > KINDA_SMALL_NUMBER ? FMath::Clamp(Current / SafeMax, 0.f, 1.f) : 0.f;
		Bar->SetPercent(Percent);
	}
}

void UDFAbilityHotbarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (InputLabels.Num() == 0)
	{
		InputLabels = {
			NSLOCTEXT("DFHUD", "HotbarInput1", "1"),
			NSLOCTEXT("DFHUD", "HotbarInput2", "2"),
			NSLOCTEXT("DFHUD", "HotbarInput3", "3"),
			NSLOCTEXT("DFHUD", "HotbarInput4", "4"),
			NSLOCTEXT("DFHUD", "HotbarInput5", "5"),
			NSLOCTEXT("DFHUD", "HotbarInput6", "6"),
			NSLOCTEXT("DFHUD", "HotbarInput7", "7"),
			NSLOCTEXT("DFHUD", "HotbarInput8", "8"),
			NSLOCTEXT("DFHUD", "HotbarInput9", "9"),
			NSLOCTEXT("DFHUD", "HotbarInput0", "0"),
			NSLOCTEXT("DFHUD", "HotbarInputMinus", "-"),
			NSLOCTEXT("DFHUD", "HotbarInputEquals", "=")
		};
	}

	CollectSlots();
	BindToPlayerCharacter();
	RefreshHotbar();
}

void UDFAbilityHotbarWidget::NativeDestruct()
{
	UnbindFromPlayerCharacter();
	Super::NativeDestruct();
}

void UDFAbilityHotbarWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (BoundPlayerCharacter.IsValid())
	{
		return;
	}

	RebindAccumulator += InDeltaTime;
	if (RebindAccumulator >= RefreshInterval)
	{
		RebindAccumulator = 0.f;
		BindToPlayerCharacter();
		if (!BoundPlayerCharacter.IsValid())
		{
			RefreshHotbar();
		}
	}
}

void UDFAbilityHotbarWidget::CollectSlots()
{
	Slots.Reset();
	const TObjectPtr<UDFAbilitySlotWidget> Ordered[] = {
		AbilitySlot1.Get() ? AbilitySlot1 : Slot1,
		AbilitySlot2.Get() ? AbilitySlot2 : Slot2,
		AbilitySlot3.Get() ? AbilitySlot3 : Slot3,
		AbilitySlot4.Get() ? AbilitySlot4 : Slot4,
		AbilitySlot5,
		AbilitySlot6,
		AbilitySlot7,
		AbilitySlot8,
		AbilitySlot9,
		AbilitySlot10,
		AbilitySlot11,
		AbilitySlot12,
	};
	for (int32 i = 0; i < UE_ARRAY_COUNT(Ordered); ++i)
	{
		if (UDFAbilitySlotWidget* const S = Ordered[i].Get())
		{
			S->SetBarSlotIndex(i);
			S->SetOwningHotbar(this);
			Slots.Add(S);
		}
	}
	LastShownAbilityRows.Init(NAME_None, Slots.Num());
}

void UDFAbilityHotbarWidget::BindToPlayerCharacter()
{
	UnbindFromPlayerCharacter();
	if (ADFPlayerCharacter* const PC = GetDFPlayerCharacter())
	{
		BoundPlayerCharacter = PC;
		PC->OnAbilityBarSlotsChanged.AddDynamic(this, &UDFAbilityHotbarWidget::HandleAbilityBarSlotsChanged);
		SetIsEnabled(true);
	}
	TryBindEmbeddedVitals();
	if (BoundPlayerCharacter.IsValid() && bEmbeddedVitalsBound)
	{
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UDFAbilityHotbarWidget::UnbindFromPlayerCharacter()
{
	if (ADFPlayerCharacter* const PC = BoundPlayerCharacter.Get())
	{
		PC->OnAbilityBarSlotsChanged.RemoveDynamic(this, &UDFAbilityHotbarWidget::HandleAbilityBarSlotsChanged);
	}
	BoundPlayerCharacter.Reset();
	UnbindAllAttributeChanges();
	bEmbeddedVitalsBound = false;
}

void UDFAbilityHotbarWidget::TryBindEmbeddedVitals()
{
	if (bEmbeddedVitalsBound || (!HealthOrb && !ManaOrb && !StaminaBar))
	{
		return;
	}
	UAbilitySystemComponent* const ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return;
	}
	const TDelegate<void(const FOnAttributeChangeData&)> Callback =
		TDelegate<void(const FOnAttributeChangeData&)>::CreateUObject(
			this, &UDFAbilityHotbarWidget::OnEmbeddedVitalChanged);
	BindToAttributeChanges(ASC, UDFAttributeSet::GetHealthAttribute(), Callback);
	BindToAttributeChanges(ASC, UDFAttributeSet::GetMaxHealthAttribute(), Callback);
	BindToAttributeChanges(ASC, UDFAttributeSet::GetManaAttribute(), Callback);
	BindToAttributeChanges(ASC, UDFAttributeSet::GetMaxManaAttribute(), Callback);
	BindToAttributeChanges(ASC, UDFAttributeSet::GetStaminaAttribute(), Callback);
	BindToAttributeChanges(ASC, UDFAttributeSet::GetMaxStaminaAttribute(), Callback);
	bEmbeddedVitalsBound = true;
	RefreshEmbeddedVitals();
}

void UDFAbilityHotbarWidget::OnEmbeddedVitalChanged(const FOnAttributeChangeData& Data)
{
	(void)Data;
	RefreshEmbeddedVitals();
}

void UDFAbilityHotbarWidget::HandleAbilityBarSlotsChanged()
{
	LastShownAbilityRows.Init(NAME_None, Slots.Num());
	RefreshHotbar();
}

void UDFAbilityHotbarWidget::RequestSwapSlots(const int32 SlotIndexA, const int32 SlotIndexB)
{
	if (ADFPlayerCharacter* const PC = GetDFPlayerCharacter())
	{
		PC->RequestSwapAbilityBarSlots(SlotIndexA, SlotIndexB);
	}
}

UDataTable* UDFAbilityHotbarWidget::ResolveAbilityDataTable() const
{
	UGameInstance* const GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}

	const UDFRunManager* const RunManager = GameInstance->GetSubsystem<UDFRunManager>();
	return RunManager ? RunManager->AbilityDataTable : nullptr;
}

void UDFAbilityHotbarWidget::RefreshEmbeddedVitals() const
{
	if (!HealthOrb && !ManaOrb && !StaminaBar)
	{
		return;
	}

	UAbilitySystemComponent* const ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		SetProgressBarPercent(HealthOrb, 0.f, 1.f);
		SetProgressBarPercent(ManaOrb, 0.f, 1.f);
		SetProgressBarPercent(StaminaBar, 0.f, 1.f);
		return;
	}

	const float Health = ASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute());
	const float MaxHealth = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
	const float Mana = ASC->GetNumericAttribute(UDFAttributeSet::GetManaAttribute());
	const float MaxMana = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxManaAttribute());
	const float Stamina = ASC->GetNumericAttribute(UDFAttributeSet::GetStaminaAttribute());
	const float MaxStamina = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxStaminaAttribute());

	SetProgressBarPercent(HealthOrb, Health, MaxHealth);
	SetProgressBarPercent(ManaOrb, Mana, MaxMana);
	SetProgressBarPercent(StaminaBar, Stamina, MaxStamina);
}

void UDFAbilityHotbarWidget::RefreshHotbar()
{
	if (Slots.Num() == 0)
	{
		CollectSlots();
	}

	ADFPlayerCharacter* const PlayerCharacter = GetDFPlayerCharacter();
	UDataTable* const AbilityDataTable = ResolveAbilityDataTable();
	if (!PlayerCharacter || !AbilityDataTable)
	{
		for (TObjectPtr<UDFAbilitySlotWidget>& HotbarSlot : Slots)
		{
			if (UDFAbilitySlotWidget* const SlotWidget = HotbarSlot.Get())
			{
				SlotWidget->ClearAbilitySlotData();
			}
		}
		LastShownAbilityRows.Init(NAME_None, Slots.Num());
		RefreshEmbeddedVitals();
		return;
	}

	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		UDFAbilitySlotWidget* const SlotWidget = Slots[SlotIndex].Get();
		if (!SlotWidget)
		{
			continue;
		}

		const FName AbilityRowName = PlayerCharacter->CurrentAbilitySlots.IsValidIndex(SlotIndex)
			? PlayerCharacter->CurrentAbilitySlots[SlotIndex]
			: NAME_None;
		if (LastShownAbilityRows.IsValidIndex(SlotIndex) && LastShownAbilityRows[SlotIndex] == AbilityRowName)
		{
			continue;
		}

		if (LastShownAbilityRows.IsValidIndex(SlotIndex))
		{
			LastShownAbilityRows[SlotIndex] = AbilityRowName;
		}

		if (AbilityRowName.IsNone())
		{
			SlotWidget->ClearAbilitySlotData();
			continue;
		}

		const FDFAbilityTableRow* const Row =
			AbilityDataTable->FindRow<FDFAbilityTableRow>(AbilityRowName, TEXT("UDFAbilityHotbarWidget::RefreshHotbar"), false);
		if (!Row)
		{
			SlotWidget->ClearAbilitySlotData();
			continue;
		}

		const FText InputLabel = InputLabels.IsValidIndex(SlotIndex)
			? InputLabels[SlotIndex]
			: FText::AsNumber(SlotIndex + 1);
		SlotWidget->SetAbilitySlotData(Row->AbilityTag, Row->Icon, Row->DisplayName, InputLabel);
	}

	RefreshEmbeddedVitals();
}
