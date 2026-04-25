// Source/DungeonForged/Public/UI/ClassSelection/UDFClassAbilityTooltipWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/UDFUserWidgetBase.h"
#include "Data/DFDataTableStructs.h"
#include "UDFClassAbilityTooltipWidget.generated.h"

class UImage;
class UTextBlock;

UCLASS()
class DUNGEONFORGED_API UDFClassAbilityTooltipWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()
public:
	void SetAbilityData(const FDFAbilityTableRow& Row, const FName RowName);
	void PositionNear(const FVector2D& ScreenPixelPos, const FVector2D& ViewportSize);
protected:
	virtual void NativeConstruct() override;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UImage> AbilityIcon;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> AbilityName;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> AbilityDescription;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> CostText;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> CooldownText;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> TagText;
};
