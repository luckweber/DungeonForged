// Source/DungeonForged/Private/UI/UDFAbilityCardWidget.cpp
#include "UI/UDFAbilityCardWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Styling/SlateColor.h"

void UDFAbilityCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (SelectButton)
	{
		SelectButton->OnClicked.AddDynamic(this, &UDFAbilityCardWidget::HandleSelectPressed);
	}
}

void UDFAbilityCardWidget::SetupCard(const FDFAbilityTableRow& Row, const FName InRowName)
{
	RowKey = InRowName;
	if (AbilityName)
	{
		AbilityName->SetText(Row.DisplayName);
	}
	if (Description)
	{
		Description->SetText(Row.Description);
	}
	if (RarityLabel)
	{
		const UEnum* E = StaticEnum<EItemRarity>();
		if (E)
		{
			const FString R = E->GetNameStringByValue(static_cast<int64>(Row.Rarity));
			RarityLabel->SetText(FText::FromString(R));
		}
		ApplyRarityStyle(Row.Rarity);
	}
	if (CooldownText)
	{
		CooldownText->SetText(Row.DisplayCooldown.IsEmpty() ? FText::GetEmpty() : Row.DisplayCooldown);
	}
	if (CostText)
	{
		CostText->SetText(Row.DisplayCost.IsEmpty() ? FText::GetEmpty() : Row.DisplayCost);
	}
	if (AbilityIcon && Row.Icon)
	{
		AbilityIcon->SetBrushFromTexture(Row.Icon, false);
	}
	if (SelectButton)
	{
		SelectButton->SetIsEnabled(true);
	}
}

void UDFAbilityCardWidget::ApplyRarityStyle(const EItemRarity Rarity) const
{
	FSlateColor C = FSlateColor::UseForeground();
	switch (Rarity)
	{
	case EItemRarity::Uncommon: C = FSlateColor(FLinearColor(0.2f, 0.85f, 0.2f));
		break;
	case EItemRarity::Rare: C = FSlateColor(FLinearColor(0.25f, 0.5f, 1.f));
		break;
	case EItemRarity::Epic:
	case EItemRarity::Legendary: C = FSlateColor(FLinearColor(0.6f, 0.3f, 0.9f));
		break;
	case EItemRarity::Common:
	default: C = FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f));
		break;
	}
	if (RarityLabel)
	{
		RarityLabel->SetColorAndOpacity(C);
	}
}

void UDFAbilityCardWidget::HandleSelectPressed()
{
	OnCardSelectClicked.Broadcast();
}
