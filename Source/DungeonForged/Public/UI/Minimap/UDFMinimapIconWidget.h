// Source/DungeonForged/Public/UI/Minimap/UDFMinimapIconWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Minimap/ADFMinimapRoom.h"
#include "UDFMinimapIconWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

UENUM(BlueprintType)
enum class EUDFMinimapIconState : uint8
{
	/** No icon. */
	Hidden		UMETA(DisplayName = "Hidden"),
	/** Faded: adjacent to a revealed room, not yet entered. */
	Lookahead	UMETA(DisplayName = "Lookahead"),
	/** Standard revealed icon. */
	Revealed	UMETA(DisplayName = "Revealed"),
};

/**
 * WBP: one 32x32 (or your layout) child on the icon canvas. Pulse when this room is the current player room.
 */
UCLASS(Blueprintable, BlueprintType)
class DUNGEONFORGED_API UDFMinimapIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void BindToRoom(ADFMinimapRoom* InRoom, UTexture2D* IconTex, FLinearColor Tint);

	void SetIconState(const EUDFMinimapIconState InState, const bool bVisited, const bool bPulsing);

	UFUNCTION(BlueprintCallable, Category = "DF|Minimap|Icon")
	void SetLabelExpanded(const bool bShowLabels, const FText& Label);

	UFUNCTION(BlueprintCallable, Category = "DF|Minimap|Icon")
	ADFMinimapRoom* GetBoundRoom() const { return Room.Get(); }

	/** Drives the current-room pulse; called from `UDFMinimapWidget::NativeTick` (UUserWidget native tick is off by default). */
	void StepPulse(const float DTM);

protected:
	TWeakObjectPtr<ADFMinimapRoom> Room;
	EUDFMinimapIconState State = EUDFMinimapIconState::Hidden;
	uint8 bVisitedLocal : 1 = false;
	uint8 bPulsingLocal : 1 = false;
	float PulsePhase = 0.f;
	FLinearColor CachedTint = FLinearColor::White;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> RoomIcon = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> VisitedOverlay = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RoomLabel = nullptr;
};
