// Source/DungeonForged/Private/Equipment/UDFCharacterScreenWidget.cpp
#include "Equipment/UDFCharacterScreenWidget.h"
#include "Equipment/UDFPreviewCaptureComponent.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Components/Image.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Styling/SlateBrush.h"

void UDFCharacterScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (ADFPlayerCharacter* const C = GetDFPlayerCharacter())
	{
		if (UDFPreviewCaptureComponent* const Cap = C->GetEquipmentPreview())
		{
			if (PaperDollRenderTarget)
			{
				Cap->SetRenderTargetForCapture(PaperDollRenderTarget);
			}
			Cap->SetPreviewTarget(C);
			Cap->SetPreviewActive(true);
		}
	}
	if (PaperDollImage && PaperDollRenderTarget)
	{
		FSlateBrush B;
		B.SetResourceObject(PaperDollRenderTarget);
		const int32 SX = FMath::Max(1, static_cast<int32>(PaperDollRenderTarget->GetSurfaceWidth()));
		const int32 SY = FMath::Max(1, static_cast<int32>(PaperDollRenderTarget->GetSurfaceHeight()));
		B.ImageSize = FVector2D(static_cast<float>(SX), static_cast<float>(SY));
		PaperDollImage->SetBrush(B);
	}
}

void UDFCharacterScreenWidget::NativeDestruct()
{
	if (ADFPlayerCharacter* const C = GetDFPlayerCharacter())
	{
		if (UDFPreviewCaptureComponent* const Cap = C->GetEquipmentPreview())
		{
			Cap->SetPreviewActive(false);
		}
	}
	Super::NativeDestruct();
}

void UDFCharacterScreenWidget::AddPreviewYawFromMousePixelDelta(const float MouseDeltaXPixels)
{
	ADFPlayerCharacter* const C = GetDFPlayerCharacter();
	if (!C)
	{
		return;
	}
	if (UDFPreviewCaptureComponent* const Cap = C->GetEquipmentPreview())
	{
		Cap->AddOrbitDeltaYaw(MouseDeltaXPixels * MouseOrbitScale);
	}
}
