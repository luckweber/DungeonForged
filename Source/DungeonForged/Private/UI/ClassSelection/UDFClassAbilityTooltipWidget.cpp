// Source/DungeonForged/Private/UI/ClassSelection/UDFClassAbilityTooltipWidget.cpp
#include "UI/ClassSelection/UDFClassAbilityTooltipWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UDFClassAbilityTooltipWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UDFClassAbilityTooltipWidget::SetAbilityData(const FDFAbilityTableRow& Row, const FName RowName)
{
	if (AbilityIcon)
	{
		if (Row.Icon) AbilityIcon->SetBrushFromTexture(Row.Icon);
	}
	if (AbilityName) AbilityName->SetText(Row.DisplayName);
	if (AbilityDescription) AbilityDescription->SetText(Row.Description);
	if (CostText) CostText->SetText(Row.DisplayCost);
	if (CooldownText) CooldownText->SetText(Row.DisplayCooldown);
	if (TagText)
	{
		const FString R = UEnum::GetDisplayValueAsText(Row.Rarity).ToString();
		TagText->SetText(FText::FromString(Row.AbilityTag.IsValid() ? Row.AbilityTag.ToString() : R));
	}
}

void UDFClassAbilityTooltipWidget::PositionNear(const FVector2D& ScreenPixelPos, const FVector2D& ViewportSize)
{
	constexpr float W = 320.f, H = 200.f;
	float X = ScreenPixelPos.X + 16.f;
	float Y = ScreenPixelPos.Y + 8.f;
	if (X + W > ViewportSize.X) X = ViewportSize.X - W - 8.f;
	if (Y + H > ViewportSize.Y) Y = ViewportSize.Y - H - 8.f;
	SetPositionInViewport(FVector2D(X, Y), false);
}
