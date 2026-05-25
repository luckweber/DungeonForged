// Source/DungeonForged/Public/Animation/AN/AnimNotifyState_LandingRecovery.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_LandingRecovery.generated.h"

/** Applies State.Landing during land recovery; blocks attack/dodge/jump. */
UCLASS(meta = (DisplayName = "DF Landing Recovery"))
class DUNGEONFORGED_API UAnimNotifyState_LandingRecovery : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
