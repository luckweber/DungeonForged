// Source/DungeonForged/Public/FX/UDFCombatFeedbackTypes.h
#pragma once

#include "CoreMinimal.h"
#include "UDFCombatFeedbackTypes.generated.h"

UENUM(BlueprintType)
enum class EDFHitFeedbackBand : uint8
{
	Light		UMETA(DisplayName = "Light"),
	Heavy		UMETA(DisplayName = "Heavy"),
	Critical	UMETA(DisplayName = "Critical (high % of max health)"),
	Knockback	UMETA(DisplayName = "Knockback"),
};
