// Source/DungeonForged/Public/GameModes/MainMenu/DFMainMenuTypes.h
#pragma once

#include "CoreMinimal.h"
#include "DFMainMenuTypes.generated.h"

UENUM(BlueprintType)
enum class EDFSlotScreenMode : uint8
{
	/** Player picks a profile to play; may continue or start new. */
	SelectToPlay  UMETA(DisplayName = "Select To Play"),
	/** Management: delete only; @c WBP_SaveSlotCard should not travel. */
	SelectToDelete UMETA(DisplayName = "Select To Delete / Manage")
};
