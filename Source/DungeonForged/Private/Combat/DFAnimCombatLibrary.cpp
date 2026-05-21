// Copyright DungeonForged. All Rights Reserved.

#include "Combat/DFAnimCombatLibrary.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

float UDFAnimCombatLibrary::PlayMontageWithBlendIn(
	UAnimInstance* AnimInstance,
	UAnimMontage* Montage,
	const float PlayRate,
	const float BlendInTime,
	const bool bStopAllMontages,
	const EAlphaBlendOption BlendOption)
{
	if (!AnimInstance || !Montage)
	{
		return 0.f;
	}

	const float ClampedBlendIn = FMath::Max(0.f, BlendInTime);
	FAlphaBlendArgs BlendIn;
	BlendIn.BlendTime = ClampedBlendIn;
	BlendIn.BlendOption = BlendOption;

	return AnimInstance->Montage_PlayWithBlendIn(
		Montage,
		BlendIn,
		PlayRate,
		EMontagePlayReturnType::MontageLength,
		0.f,
		bStopAllMontages);
}
