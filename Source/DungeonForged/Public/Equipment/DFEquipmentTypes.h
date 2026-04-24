// Source/DungeonForged/Public/Equipment/DFEquipmentTypes.h
#pragma once

#include "CoreMinimal.h"
#include "DFEquipmentTypes.generated.h"

UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	None	UMETA(Hidden),
	Weapon	UMETA(DisplayName = "Weapon"),
	OffHand	UMETA(DisplayName = "OffHand"),
	Helmet	UMETA(DisplayName = "Helmet"),
	Chest	UMETA(DisplayName = "Chest"),
	Legs	UMETA(DisplayName = "Legs"),
	Boots	UMETA(DisplayName = "Boots"),
	Gloves	UMETA(DisplayName = "Gloves"),
	Ring1	UMETA(DisplayName = "Ring1"),
	Ring2	UMETA(DisplayName = "Ring2"),
	Amulet	UMETA(DisplayName = "Amulet"),
};

static constexpr int32 EEquipmentSlotGameplayCount = static_cast<int32>(EEquipmentSlot::Amulet);
static constexpr int32 EEquipmentSlotCount = EEquipmentSlotGameplayCount + 1; // 0..Amulet
