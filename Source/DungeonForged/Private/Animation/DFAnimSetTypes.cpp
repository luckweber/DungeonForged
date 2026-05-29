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

UAnimSequenceBase* FUDAnimSet::ResolveJumpDoubleStart() const
{
	if (JumpSet.DoubleJump_Start)
	{
		return JumpSet.DoubleJump_Start;
	}
	return ResolveJumpStart(EDFMovementDirection::None);
}

UAnimSequenceBase* FUDAnimSet::ResolveJumpDoubleLoop() const
{
	if (JumpSet.DoubleJump_Loop)
	{
		return JumpSet.DoubleJump_Loop;
	}
	return ResolveJumpLoop();
}

namespace
{
template <typename TPicker>
UAnimSequenceBase* ResolveLocoDir(const FUDLocomotionAnimSet& Set, const EDFMovementDirection Dir, TPicker&& Pick)
{
	switch (Dir)
	{
	case EDFMovementDirection::Forward:        return Pick(Set, EDFMovementDirection::Forward);
	case EDFMovementDirection::ForwardRight:   return Pick(Set, EDFMovementDirection::ForwardRight);
	case EDFMovementDirection::Right:          return Pick(Set, EDFMovementDirection::Right);
	case EDFMovementDirection::BackwardRight:  return Pick(Set, EDFMovementDirection::BackwardRight);
	case EDFMovementDirection::Backward:       return Pick(Set, EDFMovementDirection::Backward);
	case EDFMovementDirection::BackwardLeft:   return Pick(Set, EDFMovementDirection::BackwardLeft);
	case EDFMovementDirection::Left:           return Pick(Set, EDFMovementDirection::Left);
	case EDFMovementDirection::ForwardLeft:    return Pick(Set, EDFMovementDirection::ForwardLeft);
	default:                                   return Pick(Set, EDFMovementDirection::Forward);
	}
}
} // namespace

UAnimSequenceBase* FUDLocomotionAnimSet::ResolveStart(const EDFMovementDirection Dir) const
{
	auto Pick = [](const FUDLocomotionAnimSet& S, const EDFMovementDirection D) -> UAnimSequenceBase*
	{
		switch (D)
		{
		case EDFMovementDirection::Forward:       return S.Start_F       ? S.Start_F       : S.Start_F;
		case EDFMovementDirection::ForwardRight:  return S.Start_FR_45   ? S.Start_FR_45   : (S.Start_F ? S.Start_F : S.Start_R_90);
		case EDFMovementDirection::Right:         return S.Start_R_90    ? S.Start_R_90    : S.Start_FR_45;
		case EDFMovementDirection::BackwardRight: return S.Start_BR_135  ? S.Start_BR_135  : (S.Start_B_180 ? S.Start_B_180 : S.Start_R_90);
		case EDFMovementDirection::Backward:      return S.Start_B_180   ? S.Start_B_180   : S.Start_F;
		case EDFMovementDirection::BackwardLeft:  return S.Start_BL_135  ? S.Start_BL_135  : (S.Start_B_180 ? S.Start_B_180 : S.Start_L_90);
		case EDFMovementDirection::Left:          return S.Start_L_90    ? S.Start_L_90    : S.Start_FL_45;
		case EDFMovementDirection::ForwardLeft:   return S.Start_FL_45   ? S.Start_FL_45   : (S.Start_F ? S.Start_F : S.Start_L_90);
		default:                                  return S.Start_F;
		}
	};
	return ResolveLocoDir(*this, Dir, Pick);
}

UAnimSequenceBase* FUDLocomotionAnimSet::ResolveLoop(const EDFMovementDirection Dir) const
{
	auto Pick = [](const FUDLocomotionAnimSet& S, const EDFMovementDirection D) -> UAnimSequenceBase*
	{
		switch (D)
		{
		case EDFMovementDirection::Forward:       return S.Loop_F       ? S.Loop_F       : S.Loop_F;
		case EDFMovementDirection::ForwardRight:  return S.Loop_FR_45   ? S.Loop_FR_45   : (S.Loop_F ? S.Loop_F : S.Loop_R_90);
		case EDFMovementDirection::Right:         return S.Loop_R_90    ? S.Loop_R_90    : S.Loop_FR_45;
		case EDFMovementDirection::BackwardRight: return S.Loop_BR_135  ? S.Loop_BR_135  : (S.Loop_B_180 ? S.Loop_B_180 : S.Loop_R_90);
		case EDFMovementDirection::Backward:      return S.Loop_B_180   ? S.Loop_B_180   : S.Loop_F;
		case EDFMovementDirection::BackwardLeft:  return S.Loop_BL_135  ? S.Loop_BL_135  : (S.Loop_B_180 ? S.Loop_B_180 : S.Loop_L_90);
		case EDFMovementDirection::Left:          return S.Loop_L_90    ? S.Loop_L_90    : S.Loop_FL_45;
		case EDFMovementDirection::ForwardLeft:   return S.Loop_FL_45   ? S.Loop_FL_45   : (S.Loop_F ? S.Loop_F : S.Loop_L_90);
		default:                                  return S.Loop_F;
		}
	};
	return ResolveLocoDir(*this, Dir, Pick);
}

UAnimSequenceBase* FUDLocomotionAnimSet::ResolveStop(const EDFMovementDirection Dir) const
{
	auto Pick = [](const FUDLocomotionAnimSet& S, const EDFMovementDirection D) -> UAnimSequenceBase*
	{
		switch (D)
		{
		case EDFMovementDirection::Forward:       return S.Stop_F       ? S.Stop_F       : S.Stop_F;
		case EDFMovementDirection::ForwardRight:  return S.Stop_FR_45   ? S.Stop_FR_45   : (S.Stop_F ? S.Stop_F : S.Stop_R_90);
		case EDFMovementDirection::Right:         return S.Stop_R_90    ? S.Stop_R_90    : S.Stop_FR_45;
		case EDFMovementDirection::BackwardRight: return S.Stop_BR_135  ? S.Stop_BR_135  : (S.Stop_B_180 ? S.Stop_B_180 : S.Stop_R_90);
		case EDFMovementDirection::Backward:      return S.Stop_B_180   ? S.Stop_B_180   : S.Stop_F;
		case EDFMovementDirection::BackwardLeft:  return S.Stop_BL_135  ? S.Stop_BL_135  : (S.Stop_B_180 ? S.Stop_B_180 : S.Stop_L_90);
		case EDFMovementDirection::Left:          return S.Stop_L_90    ? S.Stop_L_90    : S.Stop_FL_45;
		case EDFMovementDirection::ForwardLeft:   return S.Stop_FL_45   ? S.Stop_FL_45   : (S.Stop_F ? S.Stop_F : S.Stop_L_90);
		default:                                  return S.Stop_F;
		}
	};
	return ResolveLocoDir(*this, Dir, Pick);
}

UAnimSequenceBase* FUDAnimSet::ResolveLocomotionStart(const EDFGait Gait, const EDFMovementDirection Dir) const
{
	const FUDLocomotionAnimSet& Set = (Gait == EDFGait::Walk) ? WalkSet : RunSet;
	if (UAnimSequenceBase* const A = Set.ResolveStart(Dir))
	{
		return A;
	}
	if (Gait != EDFGait::Walk)
	{
		return WalkSet.ResolveStart(Dir);
	}
	return nullptr;
}

UAnimSequenceBase* FUDAnimSet::ResolveLocomotionLoop(const EDFGait Gait, const EDFMovementDirection Dir) const
{
	const FUDLocomotionAnimSet& Set = (Gait == EDFGait::Walk) ? WalkSet : RunSet;
	if (UAnimSequenceBase* const A = Set.ResolveLoop(Dir))
	{
		return A;
	}
	if (Gait != EDFGait::Walk)
	{
		return WalkSet.ResolveLoop(Dir);
	}
	return nullptr;
}

UAnimSequenceBase* FUDAnimSet::ResolveLocomotionStop(const EDFGait Gait, const EDFMovementDirection Dir) const
{
	const FUDLocomotionAnimSet& Set = (Gait == EDFGait::Walk) ? WalkSet : RunSet;
	if (UAnimSequenceBase* const A = Set.ResolveStop(Dir))
	{
		return A;
	}
	if (Gait != EDFGait::Walk)
	{
		return WalkSet.ResolveStop(Dir);
	}
	return nullptr;
}
