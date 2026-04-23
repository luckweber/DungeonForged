// Source/DungeonForged/Public/Combat/DFGroundTargetActor.h
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor_GroundTrace.h"
#include "DFGroundTargetActor.generated.h"

class UDecalComponent;

/**
 * Ground targeting reticle for AoE (Blizzard). CollisionRadius on parent defaults to 800 (16m / 2 for placement).
 * Override decal material in editor.
 */
UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFGroundTargetActor : public AGameplayAbilityTargetActor_GroundTrace
{
	GENERATED_BODY()

public:
	ADFGroundTargetActor();

	UPROPERTY(VisibleAnywhere, Category = "DF|Targeting")
	TObjectPtr<UDecalComponent> RangeDecal = nullptr;
};
