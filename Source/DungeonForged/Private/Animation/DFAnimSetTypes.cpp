// Source/DungeonForged/Private/Animation/DFAnimSetTypes.cpp
#include "Animation/DFAnimSetTypes.h"

UAnimSequenceBase* FUDJumpAnimSet::ResolveStart(const EDFMovementDirection Dir) const
{
	switch (Dir)
	{
	case EDFMovementDirection::Forward:
		return Start_Forward ? Start_Forward : Start_Idle;
	case EDFMovementDirection::Backward:
		return Start_Backward ? Start_Backward : Start_Idle;
	case EDFMovementDirection::Left:
		return Start_Left ? Start_Left : Start_Idle;
	case EDFMovementDirection::Right:
		return Start_Right ? Start_Right : Start_Idle;
	case EDFMovementDirection::ForwardLeft:
		return Start_Forward ? Start_Forward : (Start_Left ? Start_Left : Start_Idle);
	case EDFMovementDirection::ForwardRight:
		return Start_Forward ? Start_Forward : (Start_Right ? Start_Right : Start_Idle);
	case EDFMovementDirection::BackwardLeft:
		return Start_Backward ? Start_Backward : (Start_Left ? Start_Left : Start_Idle);
	case EDFMovementDirection::BackwardRight:
		return Start_Backward ? Start_Backward : (Start_Right ? Start_Right : Start_Idle);
	default:
		return Start_Idle;
	}
}

UAnimSequenceBase* FUDJumpAnimSet::ResolveLand(const EDFMovementDirection Dir) const
{
	switch (Dir)
	{
	case EDFMovementDirection::Forward:
		return Land_Forward ? Land_Forward : Land_Idle;
	case EDFMovementDirection::Backward:
		return Land_Backward ? Land_Backward : Land_Idle;
	case EDFMovementDirection::Left:
		return Land_Left ? Land_Left : Land_Idle;
	case EDFMovementDirection::Right:
		return Land_Right ? Land_Right : Land_Idle;
	case EDFMovementDirection::ForwardLeft:
		return Land_Forward ? Land_Forward : (Land_Left ? Land_Left : Land_Idle);
	case EDFMovementDirection::ForwardRight:
		return Land_Forward ? Land_Forward : (Land_Right ? Land_Right : Land_Idle);
	case EDFMovementDirection::BackwardLeft:
		return Land_Backward ? Land_Backward : (Land_Left ? Land_Left : Land_Idle);
	case EDFMovementDirection::BackwardRight:
		return Land_Backward ? Land_Backward : (Land_Right ? Land_Right : Land_Idle);
	default:
		return Land_Idle;
	}
}

UAnimSequenceBase* FUDAnimSet::ResolveJumpStart(const EDFMovementDirection Dir) const
{
	if (JumpSet.IsValid())
	{
		if (UAnimSequenceBase* const Resolved = JumpSet.ResolveStart(Dir))
		{
			return Resolved;
		}
	}
	return JumpStartAnim;
}

UAnimSequenceBase* FUDAnimSet::ResolveJumpLand(const EDFMovementDirection Dir) const
{
	if (JumpSet.IsValid())
	{
		if (UAnimSequenceBase* const Resolved = JumpSet.ResolveLand(Dir))
		{
			return Resolved;
		}
	}
	return JumpLandAnim;
}

UAnimSequenceBase* FUDAnimSet::ResolveJumpLoop() const
{
	if (JumpSet.Loop)
	{
		return JumpSet.Loop;
	}
	return JumpLoopAnim;
}
