// Source/DungeonForged/Public/Localization/DFLocalizationTypes.h
#pragma once

#include "CoreMinimal.h"
#include "DFLocalizationTypes.generated.h"

/** Supported spoken UI languages (culture + StringTable set). Default: Portuguese (Brazil). */
UENUM(BlueprintType)
enum class EDFLanguage : uint8
{
	PortugueseBrazil UMETA(DisplayName = "Portuguese (Brazil)"),
	English UMETA(DisplayName = "English"),
	Spanish UMETA(DisplayName = "Spanish"),
	French UMETA(DisplayName = "French"),
};
