// Source/DungeonForged/Public/Combat/AN/AN_HitConfirm.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_HitConfirm.generated.h"

class UAnimSequenceBase;
class USkeletalMeshComponent;
struct FAnimNotifyEventReference;

/** Sync frame for hit confirmation / trace pulse (B14). Fires Event.Combat.HitConfirm on the owner ASC. */
UCLASS(meta = (DisplayName = "DF Hit Confirm"))
class DUNGEONFORGED_API UAN_HitConfirm : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	/** If true, forces one melee trace sample this frame (when @c UDFMeleeTraceComponent is tracing). */
	UPROPERTY(EditAnywhere, Category = "DF|Combat")
	bool bForceTracePulse = true;
};
