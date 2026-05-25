// Source/DungeonForged/Public/Animation/AN/AnimNotify_JumpApex.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_JumpApex.generated.h"

/** Fired at jump apex on Jump_Loop — hook VFX/SFX in Blueprint. */
UCLASS(meta = (DisplayName = "DF Jump Apex"))
class DUNGEONFORGED_API UAnimNotify_JumpApex : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
