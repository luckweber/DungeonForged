// Source/DungeonForged/Private/Animation/UDFAnimNotify_DisableRootMotion.cpp
#include "Animation/UDFAnimNotify_DisableRootMotion.h"
#include "Animation/UDFAnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"

void UUDFAnimNotify_DisableRootMotion::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	(void)Animation;
	(void)EventReference;
	if (!MeshComp)
	{
		return;
	}
	if (UUDFAnimInstance* DFA = Cast<UUDFAnimInstance>(MeshComp->GetAnimInstance()))
	{
		DFA->PopAnimNotifiedCustomMovement();
	}
}
