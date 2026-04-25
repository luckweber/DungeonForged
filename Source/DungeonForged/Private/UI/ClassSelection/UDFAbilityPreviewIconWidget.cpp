// Source/DungeonForged/Private/UI/ClassSelection/UDFAbilityPreviewIconWidget.cpp
#include "UI/ClassSelection/UDFAbilityPreviewIconWidget.h"
#include "UI/ClassSelection/UDFClassAbilityTooltipWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UDFAbilityPreviewIconWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (HitButton)
	{
		HitButton->OnHovered.AddDynamic(this, &UDFAbilityPreviewIconWidget::OnHovered);
		HitButton->OnUnhovered.AddDynamic(this, &UDFAbilityPreviewIconWidget::OnUnhovered);
	}
}

void UDFAbilityPreviewIconWidget::SetAbilityRow(
	const FDFAbilityTableRow& Row, const FName InRowName, UDFClassAbilityTooltipWidget* SharedTooltip)
{
	CachedRow = Row;
	RowName = InRowName;
	Tooltip = SharedTooltip;
	if (AbilityIcon && Row.Icon) AbilityIcon->SetBrushFromTexture(Row.Icon);
	if (AbilityName) AbilityName->SetText(Row.DisplayName);
}

void UDFAbilityPreviewIconWidget::OnHovered()
{
	if (!Tooltip) return;
	APlayerController* const PC = GetOwningPlayer();
	if (!PC) return;
	Tooltip->SetAbilityData(CachedRow, RowName);
	Tooltip->SetVisibility(ESlateVisibility::HitTestInvisible);
	float X, Y;
	PC->GetMousePosition(X, Y);
	const FVector2D Size = UWidgetLayoutLibrary::GetViewportSize(this);
	Tooltip->PositionNear(FVector2D(X, Y), Size);
}

void UDFAbilityPreviewIconWidget::OnUnhovered()
{
	if (Tooltip) Tooltip->SetVisibility(ESlateVisibility::Collapsed);
}
