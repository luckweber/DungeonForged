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
	const EAlphaBlendOption BlendOption,
	const float InTimeToStartMontageAt)
{
	if (!AnimInstance || !Montage)
	{
		return 0.f;
	}

	const float ClampedBlendIn = FMath::Max(0.f, BlendInTime);
	FAlphaBlendArgs BlendIn;
	BlendIn.BlendTime = ClampedBlendIn;
	BlendIn.BlendOption = BlendOption;

	const float MontageLen = Montage->GetPlayLength();
	const float ClampedStartTime = MontageLen > KINDA_SMALL_NUMBER
		? FMath::Clamp(InTimeToStartMontageAt, 0.f, FMath::Max(0.f, MontageLen - KINDA_SMALL_NUMBER))
		: FMath::Max(0.f, InTimeToStartMontageAt);

	return AnimInstance->Montage_PlayWithBlendIn(
		Montage,
		BlendIn,
		PlayRate,
		EMontagePlayReturnType::MontageLength,
		ClampedStartTime,
		bStopAllMontages);
}
