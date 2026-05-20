// Source/DungeonForged/Public/Combat/AN/ANS_DFInterruptibleCast.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_DFInterruptibleCast.generated.h"

class UAnimSequenceBase;
class USkeletalMeshComponent;
struct FAnimNotifyEventReference;

/**
 * Marks a boss cast window as CC-interruptible (Shield Bash, stun, etc.).
 * Adds @c State.Combat.Casting.Interruptible on the owner ASC for the notify duration.
 */
UCLASS(Blueprintable, meta = (DisplayName = "DF Interruptible Cast"))
class DUNGEONFORGED_API UANS_DFInterruptibleCast : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UANS_DFInterruptibleCast();

	virtual FString GetNotifyName_Implementation() const override;
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
