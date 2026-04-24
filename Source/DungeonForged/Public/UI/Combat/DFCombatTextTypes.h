// Source/DungeonForged/Public/UI/Combat/DFCombatTextTypes.h
#pragma once

#include "CoreMinimal.h"
#include "DFCombatTextTypes.generated.h"

/** Floating combat / feedback text style (drives color, size, and formatting). */
UENUM(BlueprintType)
enum class ECombatTextType : uint8
{
	Damage_Physical UMETA(DisplayName = "Damage (Physical)"),
	Damage_Magic      UMETA(DisplayName = "Damage (Magic)"),
	Damage_True       UMETA(DisplayName = "Damage (True)"),
	Damage_Critical   UMETA(DisplayName = "Damage (Critical)"),
	Damage_DoT        UMETA(DisplayName = "Damage (DoT)"),
	Heal              UMETA(DisplayName = "Heal"),
	Mana_Restore      UMETA(DisplayName = "Mana Restore"),
	Miss              UMETA(DisplayName = "Miss"),
	Dodge             UMETA(DisplayName = "Dodge"),
	Block             UMETA(DisplayName = "Block"),
	Immune            UMETA(DisplayName = "Immune"),
	LevelUp           UMETA(DisplayName = "Level Up"),
	XPGain            UMETA(DisplayName = "XP Gain"),
	GoldGain          UMETA(DisplayName = "Gold Gain"),
	Status            UMETA(DisplayName = "Status (Debuff name)"),
};
