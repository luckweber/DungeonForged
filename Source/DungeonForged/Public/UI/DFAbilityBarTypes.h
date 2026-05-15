// Source/DungeonForged/Public/UI/DFAbilityBarTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "DFAbilityBarTypes.generated.h"

/** WoW-style action bar slot count (keys 1-9, 0, -, =). */
inline constexpr int32 DFAbilityBarSlotCount = 12;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDFOnAbilityBarSlotsChanged);

UCLASS()
class DUNGEONFORGED_API UDFAbilityBarDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "DF|UI|AbilityBar")
	int32 SourceSlotIndex = INDEX_NONE;
};
