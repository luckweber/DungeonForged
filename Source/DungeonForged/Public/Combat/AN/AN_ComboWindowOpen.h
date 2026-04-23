// Source/DungeonForged/Public/Combat/AN/AN_ComboWindowOpen.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_ComboWindowOpen.generated.h"

UCLASS(Blueprintable, meta = (DisplayName = "DF Combo Window (Advance Combo)"))
class DUNGEONFORGED_API UAN_ComboWindowOpen : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
