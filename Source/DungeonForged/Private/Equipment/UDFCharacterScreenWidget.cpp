// Source/DungeonForged/Private/Equipment/UDFCharacterScreenWidget.cpp
#include "Equipment/UDFCharacterScreenWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Components/Image.h"
#include "DFInventoryComponent.h"
#include "Equipment/UDFEquipmentSlotWidget.h"
#include "Equipment/UDFEquipmentComponent.h"
#include "Equipment/UDFInventorySlotWidget.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameModes/Run/ADFRunPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "Input/Events.h"
#include "Styling/SlateBrush.h"

// ─────────────────────────────────────────────────────────────────────────────
// NativeOnInitialized
// Called once during widget construction, before the owning player controller
// is accessible. Safe only for one-time setup that does NOT require the player
// character (e.g. binding static delegates, reading CDO data).
// DO NOT call RefreshAllGearSlotWidgets here – the player character is not yet
// reachable and GetDFPlayerCharacter() will return nullptr.
// ─────────────────────────────────────────────────────────────────────────────
void UDFCharacterScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// Static / CDO-level init only. Character-dependent refresh deferred to
	// NativeConstruct where the owning controller is available.
}

// ─────────────────────────────────────────────────────────────────────────────
// NativeConstruct
// Called every time the widget is added to the viewport. The owning player
// controller (and therefore the player character) is accessible here.
// ─────────────────────────────────────────────────────────────────────────────
void UDFCharacterScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// BUG FIX: listeners must be bound BEFORE the first refresh so that any
	// changes that arrive during the refresh are not missed.
	BindInventoryAndEquipmentListeners();

	// Now safe to refresh – player character is available.
	RefreshAllGearSlotWidgets();

	// ── Spawn paper-doll preview actor ───────────────────────────────────────
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	TSubclassOf<ADFEquipmentPreviewActor> const ClassToSpawn = PreviewActorClass
		? PreviewActorClass
		: TSubclassOf<ADFEquipmentPreviewActor>(ADFEquipmentPreviewActor::StaticClass());

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	PreviewActor = World->SpawnActor<ADFEquipmentPreviewActor>(
		ClassToSpawn,
		FVector(0.f, 99999.f, 0.f),
		FRotator::ZeroRotator,
		Params);

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
		B.ImageSize     = FVector2D(static_cast<float>(SX), static_cast<float>(SY));
		PaperDollImage->SetBrush(B);
	}
}

void UDFCharacterScreenWidget::NativeDestruct()
{
	UnbindInventoryAndEquipmentListeners();
	if (PreviewActor)
	{
		PreviewActor->SetPreviewActive(false);
		PreviewActor->Destroy();
		PreviewActor = nullptr;
	}
	Super::NativeDestruct();
}

// ─────────────────────────────────────────────────────────────────────────────
// Listener binding
// ─────────────────────────────────────────────────────────────────────────────
void UDFCharacterScreenWidget::BindInventoryAndEquipmentListeners()
{
	ADFPlayerCharacter* const C = GetDFPlayerCharacter();
	if (!C)
	{
		return;
	}

	// BUG FIX: the original code set bInventoryEquipmentListenersBound = true
	// as soon as either component was found. If one component was missing the
	// flag still fired, blocking a future retry. Now we track the two components
	// independently so a retry is possible if one is temporarily unavailable.

	if (!bInventoryListenerBound)
	{
		if (UDFInventoryComponent* const Inv = C->GetDFInventory())
		{
			Inv->OnInventoryChanged.AddDynamic(
				this, &UDFCharacterScreenWidget::HandleInventoryChangedForScreen);
			bInventoryListenerBound = true;
		}
	}

	if (!bEquipmentListenerBound)
	{
		if (UDFEquipmentComponent* const Eq = C->GetDFEquipment())
		{
			Eq->OnEquipmentChanged.AddDynamic(
				this, &UDFCharacterScreenWidget::HandleEquipmentChangedForScreen);
			bEquipmentListenerBound = true;
		}
	}
}

void UDFCharacterScreenWidget::UnbindInventoryAndEquipmentListeners()
{
	ADFPlayerCharacter* const C = GetDFPlayerCharacter();

	if (bInventoryListenerBound)
	{
		if (C)
		{
			if (UDFInventoryComponent* const Inv = C->GetDFInventory())
			{
				Inv->OnInventoryChanged.RemoveDynamic(
					this, &UDFCharacterScreenWidget::HandleInventoryChangedForScreen);
			}
		}
		bInventoryListenerBound = false;
	}

	if (bEquipmentListenerBound)
	{
		if (C)
		{
			if (UDFEquipmentComponent* const Eq = C->GetDFEquipment())
			{
				Eq->OnEquipmentChanged.RemoveDynamic(
					this, &UDFCharacterScreenWidget::HandleEquipmentChangedForScreen);
			}
		}
		bEquipmentListenerBound = false;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Refresh helpers
// ─────────────────────────────────────────────────────────────────────────────
void UDFCharacterScreenWidget::RefreshAllGearSlotWidgets()
{
	if (!WidgetTree)
	{
		return;
	}
	WidgetTree->ForEachWidgetAndDescendants([](UWidget* const W)
	{
		if (UDFEquipmentSlotWidget* const E = Cast<UDFEquipmentSlotWidget>(W))
		{
			E->RefreshFromEquipment();
		}
		else if (UDFInventorySlotWidget* const I = Cast<UDFInventorySlotWidget>(W))
		{
			I->RefreshFromInventory();
		}
	});
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

// ─────────────────────────────────────────────────────────────────────────────
// Event handlers
// ─────────────────────────────────────────────────────────────────────────────
void UDFCharacterScreenWidget::HandleInventoryChangedForScreen()
{
	RefreshPaperDollFromOwner();
	OnCharacterScreenEquipmentOrInventoryChanged();
}

void UDFCharacterScreenWidget::HandleEquipmentChangedForScreen(
	const EEquipmentSlot ChangedSlot, const FName ItemRow)
{
	(void)ChangedSlot;
	(void)ItemRow;
	RefreshPaperDollFromOwner();
	OnCharacterScreenEquipmentOrInventoryChanged();
}

// ─────────────────────────────────────────────────────────────────────────────
// Input
// ─────────────────────────────────────────────────────────────────────────────
FReply UDFCharacterScreenWidget::NativeOnPreviewKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	const FKey K = InKeyEvent.GetKey();
	if (K == EKeys::Escape || K == EKeys::I)
	{
		if (APlayerController* const PC = GetOwningPlayer())
		{
			if (ADFRunPlayerController* const RunPC = Cast<ADFRunPlayerController>(PC))
			{
				RunPC->ToggleInventory();
				return FReply::Handled();
			}
		}
	}
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

// ─────────────────────────────────────────────────────────────────────────────
// Paper-doll orbit
// ─────────────────────────────────────────────────────────────────────────────
void UDFCharacterScreenWidget::AddPreviewYawFromMousePixelDelta(const float MouseDeltaXPixels)
{
	if (PreviewActor)
	{
		PreviewActor->AddOrbitDeltaYaw(MouseDeltaXPixels * MouseOrbitScale);
	}
}