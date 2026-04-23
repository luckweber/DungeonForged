// Source/DungeonForged/Public/Combat/AN/AN_TraceStart.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_TraceStart.generated.h"

UCLASS(Blueprintable, meta = (DisplayName = "DF Melee Trace Start"))
class DUNGEONFORGED_API UAN_TraceStart : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
