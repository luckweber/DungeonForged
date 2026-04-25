// Source/DungeonForged/Public/UI/ClassSelection/UDFPartnerClassPreviewWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFPartnerClassPreviewWidget.generated.h"

class UImage;
class UTextBlock;

/** Prompt 70: co-op partner class thumbnail; update from GameState. */
UCLASS()
class DUNGEONFORGED_API UDFPartnerClassPreviewWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()
protected:
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UImage> PartnerThumbnail;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PartnerClassLabel;
};
