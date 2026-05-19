// Copyright DungeonForged. All Rights Reserved.

#include "Combat/DFAnimCombatLibrary.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimationTypes.h"

float UDFAnimCombatLibrary::PlayMontageWithBlendIn(
	UAnimInstance* AnimInstance,
	UAnimMontage* Montage,
	const float PlayRate,
	const float BlendInTime,
	const bool bStopAllMontages)
{
	if (!AnimInstance || !Montage)
	{
		return 0.f;
	}

	const float ClampedBlendIn = FMath::Max(0.f, BlendInTime);
	const FAlphaBlendArgs BlendIn(ClampedBlendIn);

	return AnimInstance->Montage_PlayWithBlendIn(
		Montage,
		BlendIn,
		PlayRate,
		EMontagePlayReturnType::MontageLength,
		0.f,
		bStopAllMontages);
}
