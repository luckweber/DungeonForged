// Source/DungeonForged/Public/GAS/Cues/UDFGameplayCueNotify_CombatImpact.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "UDFGameplayCueNotify_CombatImpact.generated.h"

/** Spawns impact VFX/SFX from @c UDFCombatTuningData using @c Impact.* tag on cue parameters. */
UCLASS()
class DUNGEONFORGED_API UDFGameplayCueNotify_CombatImpact : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UDFGameplayCueNotify_CombatImpact();

protected:
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const override;
};
