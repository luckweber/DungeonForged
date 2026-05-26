// Source/DungeonForged/Private/Animation/AN/AnimNotify_JumpApex.cpp
#include "Animation/AN/AnimNotify_JumpApex.h"

#include "Combat/DFJumpDebug.h"
#include "Components/SkeletalMeshComponent.h"
#include "DungeonForgedModule.h"

void UAnimNotify_JumpApex::Notify(USkeletalMeshComponent* const MeshComp, UAnimSequenceBase* const Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	DFJumpDebug::Logf(TEXT("Apex notify owner=%s"), *GetNameSafe(MeshComp ? MeshComp->GetOwner() : nullptr));
}
