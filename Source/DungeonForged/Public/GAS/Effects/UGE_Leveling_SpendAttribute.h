// Source/DungeonForged/Public/GAS/Effects/UGE_Leveling_SpendAttribute.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Leveling_SpendAttribute.generated.h"

/** Instant: Strength += SetByCaller Data.Magnitude. */
UCLASS()
class DUNGEONFORGED_API UGE_Leveling_Spend_Strength : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Leveling_Spend_Strength();
};

/** Instant: Intelligence += SetByCaller Data.Magnitude. */
UCLASS()
class DUNGEONFORGED_API UGE_Leveling_Spend_Intelligence : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Leveling_Spend_Intelligence();
};

/** Instant: Agility += SetByCaller Data.Magnitude. */
UCLASS()
class DUNGEONFORGED_API UGE_Leveling_Spend_Agility : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Leveling_Spend_Agility();
};
