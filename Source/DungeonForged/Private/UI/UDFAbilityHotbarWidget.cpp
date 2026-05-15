// Source/DungeonForged/Private/UI/UDFAbilityHotbarWidget.cpp
#include "UI/UDFAbilityHotbarWidget.h"
#include "AbilitySystemComponent.h"
#include "Characters/ADFPlayerCharacter.h"
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
			NSLOCTEXT("DFHUD", "HotbarInput4", "4")
		};
	}

	CollectSlots();
	RefreshHotbar();
}

void UDFAbilityHotbarWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshAccumulator += InDeltaTime;
	if (RefreshAccumulator >= RefreshInterval)
	{
		RefreshAccumulator = 0.f;
		RefreshHotbar();
	}
}

void UDFAbilityHotbarWidget::CollectSlots()
{
	Slots.Reset();
	Slots.Add(AbilitySlot1.Get() ? AbilitySlot1 : Slot1);
	Slots.Add(AbilitySlot2.Get() ? AbilitySlot2 : Slot2);
	Slots.Add(AbilitySlot3.Get() ? AbilitySlot3 : Slot3);
	Slots.Add(AbilitySlot4.Get() ? AbilitySlot4 : Slot4);
	LastShownAbilityRows.Init(NAME_None, Slots.Num());
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
