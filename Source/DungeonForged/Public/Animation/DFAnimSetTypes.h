// Source/DungeonForged/Public/Animation/DFAnimSetTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/BlendSpace.h"
#include "DFAnimSetTypes.generated.h"

/** Animation bundle for idle / locomotion / jump driven by AnimGraph (Break → Blend Space Player, etc.). */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FUDAnimSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Idle")
	TObjectPtr<UAnimSequenceBase> IdleAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Locomotion")
	TObjectPtr<UBlendSpace> MovementBlendSpace;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Jump")
	TObjectPtr<UAnimSequenceBase> JumpStartAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Jump")
	TObjectPtr<UAnimSequenceBase> JumpLoopAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Jump")
	TObjectPtr<UAnimSequenceBase> JumpLandAnim;

	/** Idle or locomotion BS is enough to treat the set as usable (Elder checks idle only). */
	bool IsValid() const
	{
		return IdleAnimation != nullptr || MovementBlendSpace != nullptr;
	}
};
