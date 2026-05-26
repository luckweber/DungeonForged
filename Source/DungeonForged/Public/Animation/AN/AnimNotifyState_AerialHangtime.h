// Source/DungeonForged/Public/Animation/AN/AnimNotifyState_AerialHangtime.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_AerialHangtime.generated.h"

/** Reduces owner gravity during aerial combo swings (DMC-style hangtime). */
UCLASS(meta = (DisplayName = "DF Aerial Hangtime"))
class DUNGEONFORGED_API UAnimNotifyState_AerialHangtime : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Aerial", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GravityScaleMultiplier = 0.4f;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
