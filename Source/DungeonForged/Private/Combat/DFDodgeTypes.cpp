// Source/DungeonForged/Private/Combat/DFDodgeTypes.cpp
#include "Combat/DFDodgeTypes.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"

bool DFMontageHasRootMotion(const UAnimMontage* Montage)
{
	if (!Montage)
	{
		return false;
	}
	if (Montage->HasRootMotion())
	{
		return true;
	}
	for (const FSlotAnimationTrack& SlotTrack : Montage->SlotAnimTracks)
	{
		for (const FAnimSegment& Segment : SlotTrack.AnimTrack.AnimSegments)
		{
			const UAnimSequenceBase* const Anim = Segment.GetAnimReference().Get();
			if (const UAnimSequence* const Seq = Cast<UAnimSequence>(Anim))
			{
				if (Seq->HasRootMotion())
				{
					return true;
				}
			}
		}
	}
	return false;
}

UAnimSequenceBase* DFGetPrimaryMontageSequence(const UAnimMontage* Montage)
{
	if (!Montage || Montage->SlotAnimTracks.Num() == 0)
	{
		return nullptr;
	}
	const FAnimTrack& Track = Montage->SlotAnimTracks[0].AnimTrack;
	if (Track.AnimSegments.Num() == 0)
	{
		return nullptr;
	}
	return Track.AnimSegments[0].GetAnimReference().Get();
}
