// Source/DungeonForged/Public/Equipment/UDFCharacterScreenWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFCharacterScreenWidget.generated.h"

class UImage;
class UTextureRenderTarget2D;
class UDFPreviewCaptureComponent;

/**
 * Paper-doll: assign Child widgets named after BindWidget, place WBP_EquipmentSlot around the
 * scene capture, and set PaperDollRenderTarget (same as UDFPreviewCaptureComponent::RenderTarget on the character).
 */
UCLASS(Abstract, Blueprintable)
class DUNGEONFORGED_API UDFCharacterScreenWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	/** Binds a render target; same asset should be assigned to the character's UDFPreviewCaptureComponent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Equipment|UI")
	TObjectPtr<UTextureRenderTarget2D> PaperDollRenderTarget = nullptr;

	/** Pixels to orbit degrees scale when driving from C++/Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Equipment|UI")
	float MouseOrbitScale = 0.15f;

	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|UI")
	void AddPreviewYawFromMousePixelDelta(float MouseDeltaXPixels);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Binds the render target image for the 2D capture. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> PaperDollImage = nullptr;
};
