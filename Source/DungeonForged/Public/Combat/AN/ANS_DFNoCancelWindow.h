// Source/DungeonForged/Public/Combat/AN/ANS_DFNoCancelWindow.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_DFNoCancelWindow.generated.h"

class UAnimSequenceBase;
class USkeletalMeshComponent;
struct FAnimNotifyEventReference;

/**
 * Commit-grade frames: while active, @c UANS_DFCancelWindow does not open the cancel window.
 * Place on windup / impact sections where the swing must not be cancelable into heavy.
 */
UCLASS(Blueprintable, meta = (DisplayName = "DF No Cancel Frames"))
class DUNGEONFORGED_API UANS_DFNoCancelWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UANS_DFNoCancelWindow();

	virtual FString GetNotifyName_Implementation() const override;
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
