// Source/DungeonForged/Public/UI/ClassSelection/UDFAbilityPreviewIconWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/UDFUserWidgetBase.h"
#include "Data/DFDataTableStructs.h"
#include "UDFAbilityPreviewIconWidget.generated.h"

class UImage;
class UTextBlock;
class UButton;
class UDFClassAbilityTooltipWidget;

UCLASS()
class DUNGEONFORGED_API UDFAbilityPreviewIconWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()
public:
	void SetAbilityRow(const FDFAbilityTableRow& Row, const FName RowName, UDFClassAbilityTooltipWidget* SharedTooltip);
protected:
	virtual void NativeConstruct() override;
	UFUNCTION() void OnHovered();
	UFUNCTION() void OnUnhovered();
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UImage> AbilityIcon;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> AbilityName;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UButton> HitButton;
	FName RowName;
	FDFAbilityTableRow CachedRow;
	UPROPERTY(Transient) TObjectPtr<UDFClassAbilityTooltipWidget> Tooltip;
};
