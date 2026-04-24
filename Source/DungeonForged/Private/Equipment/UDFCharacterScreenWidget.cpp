// Source/DungeonForged/Private/Equipment/UDFCharacterScreenWidget.cpp
#include "Equipment/UDFCharacterScreenWidget.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Components/Image.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Styling/SlateBrush.h"

void UDFCharacterScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	TSubclassOf<ADFEquipmentPreviewActor> ClassToSpawn = PreviewActorClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = ADFEquipmentPreviewActor::StaticClass();
	}

	const FVector HiddenLocation(0.f, 99999.f, 0.f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	PreviewActor = World->SpawnActor<ADFEquipmentPreviewActor>(ClassToSpawn, HiddenLocation, FRotator::ZeroRotator, Params);
	if (!PreviewActor)
	{
		return;
	}

	if (ADFPlayerCharacter* const C = GetDFPlayerCharacter())
	{
		PreviewActor->SyncMeshFromCharacter(C);
	}

	if (PaperDollRenderTarget)
	{
		PreviewActor->InitializePreview(PaperDollRenderTarget);
	}
	PreviewActor->SetPreviewActive(true);

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
	if (PreviewActor)
	{
		PreviewActor->SetPreviewActive(false);
		PreviewActor->Destroy();
		PreviewActor = nullptr;
	}
	Super::NativeDestruct();
}

void UDFCharacterScreenWidget::RefreshPaperDollFromOwner()
{
	if (!PreviewActor)
	{
		return;
	}
	if (ADFPlayerCharacter* const C = GetDFPlayerCharacter())
	{
		PreviewActor->SyncMeshFromCharacter(C);
	}
}

void UDFCharacterScreenWidget::AddPreviewYawFromMousePixelDelta(const float MouseDeltaXPixels)
{
	if (PreviewActor)
	{
		PreviewActor->AddOrbitDeltaYaw(MouseDeltaXPixels * MouseOrbitScale);
	}
}
