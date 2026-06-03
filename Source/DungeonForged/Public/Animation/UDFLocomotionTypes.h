// Source/DungeonForged/Public/Animation/UDFLocomotionTypes.h
#pragma once

#include "CoreMinimal.h"
#include "UDFLocomotionTypes.generated.h"

/** 4-way and 8-way strafe direction from velocity relative to character facing. */
UENUM(BlueprintType)
enum class EDFMovementDirection : uint8
{
	None UMETA(Hidden),
	Forward,
	ForwardRight,
	Right,
	BackwardRight,
	Backward,
	BackwardLeft,
	Left,
	ForwardLeft
};

/** Locomotion gait: drives which 8-way Start/Loop/Stop set is chosen. */
UENUM(BlueprintType)
enum class EDFGait : uint8
{
	Idle,
	Walk,
	Run,
	Sprint
};
