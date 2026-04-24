// Source/DungeonForged/Public/UI/Minimap/UDFMinimapWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/UDFUserWidgetBase.h"
#include "UI/Minimap/ADFMinimapRoom.h"
#include "UI/Minimap/UDFMinimapIconWidget.h"
#include "UDFMinimapWidget.generated.h"

class UCanvasPanel;
class UImage;
class USizeBox;
class UTextureRenderTarget2D;
class UDFMinimapCaptureComponent;
class UDFDungeonManager;

/**
 * C++ base for WBP_Minimap. Corner minimap; expand in-place for full map. Bind: MinimapTexture, PlayerDot, IconLayer.
 * Optional: MinimapSizeBox for 200 → 500 expansion.
 */
UCLASS(Blueprintable, BlueprintType)
class DUNGEONFORGED_API UDFMinimapWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "DF|Minimap|Widget")
	void UpdateMinimapTexture();

	UFUNCTION(BlueprintCallable, Category = "DF|Minimap|Widget")
	void UpdatePlayerRotation(const float YawDegrees);

	/** True = larger panel + room labels. Interpolated in `NativeTick`. */
	UFUNCTION(BlueprintCallable, Category = "DF|Minimap|Widget")
	void SetMinimapExpanded(const bool bExpanded);

	UFUNCTION(BlueprintCallable, Category = "DF|Minimap|Widget")
	void ToggleMinimapExpanded() { SetMinimapExpanded(!bIsExpanded); }

	/** Ortho and zoom: smaller value = more zoomed in. */
	UFUNCTION(BlueprintCallable, Category = "DF|Minimap|Widget")
	void SetZoomLevel(const float InZoom);

	/** Sync icon positions, lookahead silhouettes, and current-room pulse. */
	UFUNCTION(BlueprintCallable, Category = "DF|Minimap|Widget")
	void RefreshAllRoomIcons();

protected:
	UFUNCTION()
	void OnRoomRevealed(ADFMinimapRoom* Room);

	UFUNCTION()
	void OnRoomVisited(ADFMinimapRoom* Room);

	UFUNCTION()
	void OnPlayerMinimapRoomChanged(ADFMinimapRoom* Room);

	void RebuildKnownRoomsList();
	UTexture2D* ResolveIconTexture(ADFMinimapRoom* Room) const;
	static FLinearColor GetTintForRoomType(EDFRoomType Type);
	void ApplyZoomToCapture();
	void UpdateExpansionLayout(const float DeltaTime);
	void PositionIcon(ADFMinimapRoom* Room, UDFMinimapIconWidget* Icon) const;
	void UpdateIconPositionsOnly();
	EUDFMinimapIconState ComputeState(ADFMinimapRoom* Room) const;

	/** Binds the corner RT from a level `UDFMinimapCaptureComponent` (assign on WBP or HUD). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Minimap|Widget")
	TObjectPtr<UDFMinimapCaptureComponent> MinimapCaptureComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Minimap|Widget|Icons")
	TSubclassOf<UDFMinimapIconWidget> IconWidgetClass = nullptr;

	/** Fallback icons by type when the room has no `GetIconTexture` / override. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Minimap|Widget|Icons")
	TMap<EDFRoomType, TObjectPtr<UTexture2D>> DefaultRoomIconsByType;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UImage> MinimapTexture = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UImage> PlayerDot = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UCanvasPanel> IconLayer = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> MinimapSizeBox = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Minimap|Widget")
	float ZoomLevel = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Minimap|Widget")
	uint8 bIsExpanded : 1 = false;

	/** Lerp: collapsed ~200, expanded ~500 (designer can tune). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Minimap|Widget|Expand")
	float CollapsedMapEdgePx = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Minimap|Widget|Expand")
	float ExpandedMapEdgePx = 500.f;

	/** Mirrors `UDFDungeonManager::RegisteredMinimapRooms` for easy Blueprint access. */
	UPROPERTY(BlueprintReadOnly, Category = "DF|Minimap|Widget")
	TArray<ADFMinimapRoom*> KnownRooms;

	TWeakObjectPtr<UDFDungeonManager> SubscribedDungeonManager;

	/** Not UPROPERTY: icons are `AddChild` on `IconLayer` and owned by the widget instance. */
	TMap<ADFMinimapRoom*, TObjectPtr<UDFMinimapIconWidget>> RoomToIcon;

	float CurrentMapEdgePx = 200.f;
	uint8 bTargetExpanded : 1 = false;
};
