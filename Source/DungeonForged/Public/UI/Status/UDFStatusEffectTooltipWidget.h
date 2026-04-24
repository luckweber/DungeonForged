// Source/DungeonForged/Public/UI/Status/UDFStatusEffectTooltipWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UDFStatusEffectTooltipWidget.generated.h"

class UTextBlock;

/** WBP_StatusTooltip — name, description, remaining time. */
UCLASS(Abstract, Blueprintable)
class DUNGEONFORGED_API UDFStatusEffectTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetTooltipContent(const FText& Name, const FText& Description, const FText& TimeLine);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DescriptionText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TimeText = nullptr;
};
