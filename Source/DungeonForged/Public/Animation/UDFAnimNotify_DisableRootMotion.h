// Source/DungeonForged/Public/Animation/UDFAnimNotify_DisableRootMotion.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "UDFAnimNotify_DisableRootMotion.generated.h"

/** Restore movement mode after UDFAnimNotify_EnableRootMotion (or set Walking on ground as fallback). */
UCLASS(Blueprintable, const, meta = (DisplayName = "DF | Disable Custom Movement (End Root Motion)"))
class DUNGEONFORGED_API UUDFAnimNotify_DisableRootMotion : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
