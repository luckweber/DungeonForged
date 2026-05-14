// Source/DungeonForged/Private/UI/UDFPlayerVitalsWidget.cpp
#include "UI/UDFPlayerVitalsWidget.h"
#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GAS/UDFAttributeSet.h"

void UDFPlayerVitalsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TryBindVitals();
	RefreshVitals();
}

void UDFPlayerVitalsWidget::NativeDestruct()
{
	bVitalsBound = false;
	Super::NativeDestruct();
}

void UDFPlayerVitalsWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bVitalsBound)
	{
		return;
	}

	RebindAccumulator += InDeltaTime;
	if (RebindAccumulator >= 0.25f)
	{
		RebindAccumulator = 0.f;
		TryBindVitals();
		RefreshVitals();
	}
}

void UDFPlayerVitalsWidget::TryBindVitals()
{
	if (bVitalsBound)
	{
		return;
	}

	UAbilitySystemComponent* const ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return;
	}

	const TDelegate<void(const FOnAttributeChangeData&)> Callback =
		TDelegate<void(const FOnAttributeChangeData&)>::CreateUObject(this, &UDFPlayerVitalsWidget::OnAnyVitalAttributeChanged);

	BindToAttributeChanges(ASC, UDFAttributeSet::GetHealthAttribute(), Callback);
	BindToAttributeChanges(ASC, UDFAttributeSet::GetMaxHealthAttribute(), Callback);
	BindToAttributeChanges(ASC, UDFAttributeSet::GetManaAttribute(), Callback);
	BindToAttributeChanges(ASC, UDFAttributeSet::GetMaxManaAttribute(), Callback);
	BindToAttributeChanges(ASC, UDFAttributeSet::GetStaminaAttribute(), Callback);
	BindToAttributeChanges(ASC, UDFAttributeSet::GetMaxStaminaAttribute(), Callback);

	bVitalsBound = true;
}

void UDFPlayerVitalsWidget::OnAnyVitalAttributeChanged(const FOnAttributeChangeData& Data)
{
	(void)Data;
	RefreshVitals();
}

void UDFPlayerVitalsWidget::RefreshVitals()
{
	UAbilitySystemComponent* const ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		SetResourceWidgets(HealthBar, HealthText, 0.f, 1.f, NSLOCTEXT("DFHUD", "HealthLabel", "HP"));
		SetResourceWidgets(HealthOrb, nullptr, 0.f, 1.f, NSLOCTEXT("DFHUD", "HealthLabelOrb", "HP"));
		SetResourceWidgets(ManaBar, ManaText, 0.f, 1.f, NSLOCTEXT("DFHUD", "ManaLabel", "Mana"));
		SetResourceWidgets(ManaOrb, nullptr, 0.f, 1.f, NSLOCTEXT("DFHUD", "ManaLabelOrb", "Mana"));
		SetResourceWidgets(StaminaBar, StaminaText, 0.f, 1.f, NSLOCTEXT("DFHUD", "StaminaLabel", "Stamina"));
		return;
	}

	const float Health = ASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute());
	const float MaxHealth = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
	const float Mana = ASC->GetNumericAttribute(UDFAttributeSet::GetManaAttribute());
	const float MaxMana = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxManaAttribute());
	const float Stamina = ASC->GetNumericAttribute(UDFAttributeSet::GetStaminaAttribute());
	const float MaxStamina = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxStaminaAttribute());

	SetResourceWidgets(HealthBar, HealthText, Health, MaxHealth, NSLOCTEXT("DFHUD", "HealthLabelRefresh", "HP"));
	SetResourceWidgets(HealthOrb, nullptr, Health, MaxHealth, NSLOCTEXT("DFHUD", "HealthLabelRefreshOrb", "HP"));
	SetResourceWidgets(ManaBar, ManaText, Mana, MaxMana, NSLOCTEXT("DFHUD", "ManaLabelRefresh", "Mana"));
	SetResourceWidgets(ManaOrb, nullptr, Mana, MaxMana, NSLOCTEXT("DFHUD", "ManaLabelRefreshOrb", "Mana"));
	SetResourceWidgets(StaminaBar, StaminaText, Stamina, MaxStamina, NSLOCTEXT("DFHUD", "StaminaLabelRefresh", "Stamina"));

	if (HealthPercentText)
	{
		const float Percent = MaxHealth > KINDA_SMALL_NUMBER ? Health / MaxHealth : 0.f;
		HealthPercentText->SetText(FText::AsPercent(FMath::Clamp(Percent, 0.f, 1.f)));
	}
	if (ManaPercentText)
	{
		const float Percent = MaxMana > KINDA_SMALL_NUMBER ? Mana / MaxMana : 0.f;
		ManaPercentText->SetText(FText::AsPercent(FMath::Clamp(Percent, 0.f, 1.f)));
	}
	if (StaminaPercentText)
	{
		const float Percent = MaxStamina > KINDA_SMALL_NUMBER ? Stamina / MaxStamina : 0.f;
		StaminaPercentText->SetText(FText::AsPercent(FMath::Clamp(Percent, 0.f, 1.f)));
	}
}

void UDFPlayerVitalsWidget::SetResourceWidgets(
	UProgressBar* const Bar,
	UTextBlock* const Text,
	const float Current,
	const float MaxValue,
	const FText& Label) const
{
	const float SafeMax = FMath::Max(MaxValue, 0.f);
	const float Percent = SafeMax > KINDA_SMALL_NUMBER ? FMath::Clamp(Current / SafeMax, 0.f, 1.f) : 0.f;
	if (Bar)
	{
		Bar->SetPercent(Percent);
	}
	if (Text)
	{
		const int32 RoundedCurrent = FMath::RoundToInt(Current);
		const int32 RoundedMax = FMath::RoundToInt(SafeMax);
		if (bShowLabelsInValueText)
		{
			Text->SetText(FText::Format(
				NSLOCTEXT("DFHUD", "LabeledVitalValue", "{0}: {1} / {2}"),
				Label,
				FText::AsNumber(RoundedCurrent),
				FText::AsNumber(RoundedMax)));
		}
		else
		{
			Text->SetText(FText::Format(
				NSLOCTEXT("DFHUD", "VitalValue", "{0} / {1}"),
				FText::AsNumber(RoundedCurrent),
				FText::AsNumber(RoundedMax)));
		}
	}
}
