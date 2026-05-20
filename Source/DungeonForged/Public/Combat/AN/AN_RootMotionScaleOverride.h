// Source/DungeonForged/Public/Combat/AN/AN_RootMotionScaleOverride.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_RootMotionScaleOverride.generated.h"

/** Scales root-motion translation for the remainder of the active montage (A4). */
UCLASS(meta = (DisplayName = "DF Root Motion Scale Override"))
class DUNGEONFORGED_API UAN_RootMotionScaleOverride : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAN_RootMotionScaleOverride();

	UPROPERTY(EditAnywhere, Category = "DF|Animation", meta = (ClampMin = "0.0"))
	float TranslationScale = 1.f;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
