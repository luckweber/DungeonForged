// Source/DungeonForged/Public/Interaction/DFInteractionTypes.h
#pragma once

#include "DFInteractionTypes.generated.h"

UENUM(BlueprintType)
enum class EDFDoorType : uint8
{
	KeyDoor		UMETA(DisplayName = "Key door"),
	LeverDoor	UMETA(DisplayName = "Lever door"),
	ExitDoor	UMETA(DisplayName = "Exit door"),
	BossDoor	UMETA(DisplayName = "Boss door")
};

UENUM(BlueprintType)
enum class EDFShrineType : uint8
{
	Healing		UMETA(DisplayName = "Healing"),
	Mana		UMETA(DisplayName = "Mana"),
	PowerUp		UMETA(DisplayName = "Power up"),
	Mystery		UMETA(DisplayName = "Mystery")
};
