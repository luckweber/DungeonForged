// Source/DungeonForged/Public/Equipment/UDFItemDragDropOperation.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Equipment/DFEquipmentTypes.h"
#include "UDFItemDragDropOperation.generated.h"

/** Set in native drag detection; distinguishes payload even if BP toggles booleans at runtime. */
UENUM(BlueprintType)
enum class EDFItemDragOrigin : uint8
{
	None UMETA(DisplayName = "Unset"),
	BagSlot UMETA(DisplayName = "BagSlot"),
	EquipmentSlot UMETA(DisplayName = "EquipmentSlot"),
};

/**
 * Payload for inventory ↔ equipment drag-and-drop.
 * Blueprint can subclass DefaultDragVisual or read these fields in OnDrop.
 */
UCLASS(BlueprintType, Blueprintable)
class DUNGEONFORGED_API UDFItemDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "DF|DragDrop")
	EDFItemDragOrigin DragOrigin = EDFItemDragOrigin::None;

	UPROPERTY(BlueprintReadWrite, Category = "DF|DragDrop")
	FName ItemRowName = NAME_None;

	/** INDEX_NONE when drag started from an equipment slot. */
	UPROPERTY(BlueprintReadWrite, Category = "DF|DragDrop")
	int32 SourceInventorySlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, Category = "DF|DragDrop")
	bool bFromEquipmentSlot = false;

	UPROPERTY(BlueprintReadWrite, Category = "DF|DragDrop")
	EEquipmentSlot SourceEquipmentSlot = EEquipmentSlot::None;
};
