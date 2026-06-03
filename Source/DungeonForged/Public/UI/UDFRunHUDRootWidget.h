// Source/DungeonForged/Public/UI/UDFRunHUDRootWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFRunHUDRootWidget.generated.h"

class UOverlay;
class UUserWidget;

/**
 * Single viewport root for run HUD layers (minimap, vitals, boss bar, counters).
 */
UCLASS(Abstract, Blueprintable)
class DUNGEONFORGED_API UDFRunHUDRootWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DF|HUD")
	void MountLayerWidget(UUserWidget* Child, int32 ZOrder);

	UFUNCTION(BlueprintCallable, Category = "DF|HUD")
	void SetLayerVisibility(UUserWidget* Child, ESlateVisibility InVisibility);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> HUDRootOverlay;
};
