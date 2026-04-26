// Source/DungeonForged/Public/GameModes/MainMenu/DFMainMenuTypes.h
#pragma once

#include "CoreMinimal.h"
#include "DFMainMenuTypes.generated.h"

UENUM(BlueprintType)
enum class EDFSlotScreenMode : uint8
{
	/** Player picks a profile to play; may continue or start new. */
	SelectToPlay  UMETA(DisplayName = "Select To Play"),
	/**
	 * “Gerenciar perfis” no menu: só apagamento / revisão, sem @c Travel.
	 * (Design doc: pode aparecer como @c SelectToDelete ou “ManageProfiles”.)
	 */
	SelectToDelete UMETA(DisplayName = "Select To Delete / Manage")
};

/** Where to travel after @c WBP_ClassSelection confirm when opened from the main menu. */
UENUM(BlueprintType)
enum class EDFMainMenuClassPickDestination : uint8
{
	None,
	/** First-time nexus entry or empty profile creation. */
	NexusFirstLaunch,
	/** Start dungeon run with selected class. */
	RunDungeon
};
