// Source/DungeonForged/Public/Equipment/UDFCharacterScreenWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Equipment/ADFEquipmentPreviewActor.h"
#include "Equipment/DFEquipmentTypes.h"
#include "Input/Reply.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFCharacterScreenWidget.generated.h"

class UImage;
class UTextureRenderTarget2D;

/**
 * Paper-doll: spawns a client-only ADFEquipmentPreviewActor (off-world), syncs modular meshes from the
 * local player, and binds PaperDollRenderTarget to the Image. No UDFPreviewCaptureComponent on the pawn.
 */
UCLASS(Abstract, Blueprintable)
	class DUNGEONFORGED_API UDFCharacterScreenWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	/** Subclass to spawn (e.g. BP child with different capture offset). Defaults to native preview if unset. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Equipment|UI")
	TSubclassOf<ADFEquipmentPreviewActor> PreviewActorClass;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Equipment|UI")
	TObjectPtr<ADFEquipmentPreviewActor> PreviewActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Equipment|UI")
	TObjectPtr<UTextureRenderTarget2D> PaperDollRenderTarget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Equipment|UI")
	float MouseOrbitScale = 0.15f;

	/** Re-copy meshes from the owning DF character (call after equip/unequip while this panel is open). */
	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|UI")
	void RefreshPaperDollFromOwner();

	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|UI")
	void AddPreviewYawFromMousePixelDelta(float MouseDeltaXPixels);

	/** Binds bag + equipment delegates; Blueprint child overrides @a OnCharacterScreenEquipmentOrInventoryChanged to refresh slot widgets. */
	UFUNCTION(BlueprintImplementableEvent, Category = "DF|Equipment|UI")
	void OnCharacterScreenEquipmentOrInventoryChanged();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Refresh all UDFEquipmentSlotWidget / UDFInventorySlotWidget in this widget tree (no Blueprint wiring needed). */
	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|UI")
	void RefreshAllGearSlotWidgets();

	void BindInventoryAndEquipmentListeners();
	void UnbindInventoryAndEquipmentListeners();

	UFUNCTION()
	void HandleInventoryChangedForScreen();

	UFUNCTION()
	void HandleEquipmentChangedForScreen(EEquipmentSlot ChangedSlot, FName ItemRow);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> PaperDollImage = nullptr;

private:
	bool bInventoryListenerBound = false;
	bool bEquipmentListenerBound = false;
};
