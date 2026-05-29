// Source/DungeonForged/Public/GAS/Cues/UDFGameplayCueNotify_ElementReaction.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "UDFGameplayCueNotify_ElementReaction.generated.h"

/** Spawns elemental reaction VFX from @c UDFElementalReactionSubsystem Niagara assets. */
UCLASS()
class DUNGEONFORGED_API UDFGameplayCueNotify_ElementReaction : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UDFGameplayCueNotify_ElementReaction();

protected:
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const override;
};
