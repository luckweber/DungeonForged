// Source/DungeonForged/Public/Animation/UDFAnimNotify_EnableRootMotion.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "UDFAnimNotify_EnableRootMotion.generated.h"

/** Stash current movement and set MOVE_Custom (sub-mode 0). Pair with UDFAnimNotify_DisableRootMotion. */
UCLASS(Blueprintable, const, meta = (DisplayName = "DF | Enable Custom Movement (Root Motion)"))
class DUNGEONFORGED_API UUDFAnimNotify_EnableRootMotion : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
