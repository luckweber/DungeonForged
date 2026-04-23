// Source/DungeonForged/Public/UI/UDFLockOnWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UDFLockOnWidget.generated.h"

class APlayerController;
class UImage;

/**
 * C++ base for WBP_LockOnIndicator. Designer: UImage (ring) named IndicatorImage, optional anim.
 */
UCLASS()
class DUNGEONFORGED_API UDFLockOnWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Projects world location to screen and places this widget in viewport space. */
	UFUNCTION(BlueprintCallable, Category = "DF|UI|LockOn")
	void UpdateScreenPosition(APlayerController* PC, FVector WorldPos);

protected:
	/** Optional: bind in WBP_LockOnIndicator; spin / pulse can be a UMG anim. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IndicatorImage;
};
