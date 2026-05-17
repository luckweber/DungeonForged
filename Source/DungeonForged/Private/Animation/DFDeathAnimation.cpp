// Source/DungeonForged/Private/Animation/DFDeathAnimation.cpp
#include "Animation/DFDeathAnimation.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

namespace DFDeathAnimation
{
float PlayDeathMontage(USkeletalMeshComponent* const Mesh, UAnimMontage* const Montage, const bool bStopOtherMontages)
{
	if (!Mesh || !Montage)
	{
		return 0.f;
	}
	UAnimInstance* const Anim = Mesh->GetAnimInstance();
	if (!Anim)
	{
		return Montage->GetPlayLength();
	}
	if (bStopOtherMontages)
	{
		Anim->Montage_Stop(0.1f);
	}
	const float Duration = Anim->Montage_Play(Montage, 1.f);
	if (FAnimMontageInstance* const Inst = Anim->GetActiveInstanceForMontage(Montage))
	{
		Inst->bEnableAutoBlendOut = false;
	}
	return Duration > KINDA_SMALL_NUMBER ? Duration : Montage->GetPlayLength();
}

void LockDeathPoseOnMesh(USkeletalMeshComponent* const Mesh, UAnimMontage* const Montage)
{
	if (!Mesh)
	{
		return;
	}
	Mesh->bPauseAnims = true;
	UAnimInstance* const Anim = Mesh->GetAnimInstance();
	if (!Anim)
	{
		return;
	}
	if (Montage && Anim->Montage_IsPlaying(Montage))
	{
		const float EndPos = FMath::Max(0.f, Montage->GetPlayLength() - 0.05f);
		Anim->Montage_SetPosition(Montage, EndPos);
		Anim->Montage_Pause(Montage);
	}
}

float GetDeathDestroyDelaySeconds(UAnimMontage* const Montage, const float MinSeconds, const float PaddingSeconds)
{
	const float Len = Montage ? Montage->GetPlayLength() : 0.f;
	return FMath::Max(MinSeconds, Len + PaddingSeconds);
}
} // namespace DFDeathAnimation
