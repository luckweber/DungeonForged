// Source/DungeonForged/Public/Combat/AN/AN_TraceEnd.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_TraceEnd.generated.h"

UCLASS(Blueprintable, meta = (DisplayName = "DF Melee Trace End"))
class DUNGEONFORGED_API UAN_TraceEnd : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
