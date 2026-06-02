// Source/DungeonForged/Private/Animation/DFAnimSetTypes.cpp
#include "Animation/DFAnimSetTypes.h"

#include "Misc/PackageName.h"
#include "Misc/Paths.h"

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

UAnimSequenceBase* FUDTurnInPlaceAnimSet::ResolveTurn(const bool bUse180, const bool bTurnRight) const
{
	if (bUse180)
	{
		if (bTurnRight)
		{
			return Turn_180_R ? Turn_180_R : Turn_90_R;
		}
		return Turn_180_L ? Turn_180_L : Turn_90_L;
	}
	if (bTurnRight)
	{
		return Turn_90_R ? Turn_90_R : Turn_180_R;
	}
	return Turn_90_L ? Turn_90_L : Turn_180_L;
}

UAnimSequenceBase* FUDAnimSet::ResolveLocomotionIdle() const
{
	return IdleAnimation;
}

UAnimSequenceBase* FUDAnimSet::ResolveLocomotionTurn(const bool bUse180, const bool bTurnRight) const
{
	return TurnSet.ResolveTurn(bUse180, bTurnRight);
}

void FUDAnimSet::MergeTurnSetFrom(const FUDTurnInPlaceAnimSet& Source)
{
	if (!Source.IsValid())
	{
		return;
	}
	if (!TurnSet.Turn_90_L && Source.Turn_90_L)
	{
		TurnSet.Turn_90_L = Source.Turn_90_L;
	}
	if (!TurnSet.Turn_90_R && Source.Turn_90_R)
	{
		TurnSet.Turn_90_R = Source.Turn_90_R;
	}
	if (!TurnSet.Turn_180_L && Source.Turn_180_L)
	{
		TurnSet.Turn_180_L = Source.Turn_180_L;
	}
	if (!TurnSet.Turn_180_R && Source.Turn_180_R)
	{
		TurnSet.Turn_180_R = Source.Turn_180_R;
	}
}

namespace DFAnimSetTurnAutoFill
{
static UAnimSequenceBase* LoadTurnAtPath(const FString& ObjectPath)
{
	if (ObjectPath.IsEmpty())
	{
		return nullptr;
	}
	return LoadObject<UAnimSequenceBase>(nullptr, *ObjectPath);
}

static void AddCandidateDirs(const FString& IdlePackageDir, TArray<FString>& OutDirs)
{
	OutDirs.AddUnique(IdlePackageDir / TEXT("09_Turn/01_Turn"));
	OutDirs.AddUnique(IdlePackageDir / TEXT("09_Turn"));
	OutDirs.AddUnique(FPaths::GetPath(IdlePackageDir) / TEXT("09_Turn/01_Turn"));
	OutDirs.AddUnique(FPaths::GetPath(IdlePackageDir) / TEXT("09_Turn"));

	FString Replaced = IdlePackageDir;
	if (Replaced.ReplaceInline(TEXT("/08_Idle"), TEXT("/09_Turn/01_Turn")) > 0)
	{
		OutDirs.AddUnique(Replaced);
	}
	Replaced = IdlePackageDir;
	if (Replaced.ReplaceInline(TEXT("Idle"), TEXT("09_Turn/01_Turn")) > 0)
	{
		OutDirs.AddUnique(Replaced);
	}
}

static UAnimSequenceBase* TryLoadNamedTurn(const TArray<FString>& Dirs, const TCHAR* AssetName)
{
	for (const FString& Dir : Dirs)
	{
		if (Dir.IsEmpty())
		{
			continue;
		}
		const FString ObjectPath = FString::Printf(TEXT("%s/%s.%s"), *Dir, AssetName, AssetName);
		if (UAnimSequenceBase* const Loaded = LoadTurnAtPath(ObjectPath))
		{
			return Loaded;
		}
	}
	return nullptr;
}
} // namespace DFAnimSetTurnAutoFill

bool FUDAnimSet::TryAutoFillTurnSetFromIdlePackagePaths()
{
	if (TurnSet.IsValid() || !IdleAnimation)
	{
		return false;
	}

	const FString IdlePackageName = IdleAnimation->GetOutermost()->GetName();
	const FString IdlePackageDir = FPackageName::GetLongPackagePath(IdlePackageName);
	TArray<FString> CandidateDirs;
	DFAnimSetTurnAutoFill::AddCandidateDirs(IdlePackageDir, CandidateDirs);

	auto AssignIfNull = [this, &CandidateDirs](TObjectPtr<UAnimSequenceBase>& Slot, const TCHAR* Name)
	{
		if (!Slot)
		{
			Slot = DFAnimSetTurnAutoFill::TryLoadNamedTurn(CandidateDirs, Name);
		}
	};

	AssignIfNull(TurnSet.Turn_90_L, TEXT("Turn_90_L_Seq"));
	AssignIfNull(TurnSet.Turn_90_R, TEXT("Turn_90_R_Seq"));
	AssignIfNull(TurnSet.Turn_180_L, TEXT("Turn_180_L_Seq"));
	AssignIfNull(TurnSet.Turn_180_R, TEXT("Turn_180_R_Seq"));
	// Fab / alternate naming without _Seq suffix
	AssignIfNull(TurnSet.Turn_90_L, TEXT("Turn_90_L"));
	AssignIfNull(TurnSet.Turn_90_R, TEXT("Turn_90_R"));
	AssignIfNull(TurnSet.Turn_180_L, TEXT("Turn_180_L"));
	AssignIfNull(TurnSet.Turn_180_R, TEXT("Turn_180_R"));

	return TurnSet.IsValid();
}

bool FUDAnimSet::TryAutoFillTurnSetFromKnownContentPaths()
{
	if (TurnSet.IsValid())
	{
		return false;
	}

	static const TCHAR* TurnDirs[] = {
		TEXT("/Game/Sword_and_Shield/Animations/Sequence2/09_Turn/01_Turn"),
		TEXT("/Game/Sword_and_Shield/Animations/Sequence/09_Turn/01_Turn"),
		TEXT("/Game/DungeonForged/Character/JSHero/Animation/Sword_and_Shield/Animations/Sequence2/09_Turn/01_Turn"),
	};
	TArray<FString> CandidateDirs;
	for (const TCHAR* Dir : TurnDirs)
	{
		CandidateDirs.AddUnique(FString(Dir));
	}

	auto AssignIfNull = [this, &CandidateDirs](TObjectPtr<UAnimSequenceBase>& Slot, const TCHAR* Name)
	{
		if (!Slot)
		{
			Slot = DFAnimSetTurnAutoFill::TryLoadNamedTurn(CandidateDirs, Name);
		}
	};

	AssignIfNull(TurnSet.Turn_90_L, TEXT("Turn_90_L_Seq"));
	AssignIfNull(TurnSet.Turn_90_R, TEXT("Turn_90_R_Seq"));
	AssignIfNull(TurnSet.Turn_180_L, TEXT("Turn_180_L_Seq"));
	AssignIfNull(TurnSet.Turn_180_R, TEXT("Turn_180_R_Seq"));

	return TurnSet.IsValid();
}
