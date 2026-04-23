// Source/DungeonForged/Public/GAS/Effects/UGE_BossPhaseStats.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_BossPhaseStats.generated.h"

/** One-shot or refreshable +Strength / +Movement for each new boss phase. */
UCLASS()
class DUNGEONFORGED_API UGE_BossPhaseStats : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_BossPhaseStats();
};
