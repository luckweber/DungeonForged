// Source/DungeonForged/Private/Animation/UDFAnimNotify_EnableRootMotion.cpp
#include "Animation/UDFAnimNotify_EnableRootMotion.h"
#include "Animation/UDFAnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"

void UUDFAnimNotify_EnableRootMotion::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
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
		DFA->PushAnimNotifiedCustomMovement();
	}
}
