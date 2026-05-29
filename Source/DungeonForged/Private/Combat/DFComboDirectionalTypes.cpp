// Source/DungeonForged/Private/Combat/DFComboDirectionalTypes.cpp
#include "Combat/DFComboDirectionalTypes.h"
#include "Animation/AnimMontage.h"

namespace
{
UAnimMontage* ResolveSoftMontage(const TSoftObjectPtr<UAnimMontage>& Soft)
{
	if (UAnimMontage* const Loaded = Soft.Get())
	{
		return Loaded;
	}
	return Soft.IsNull() ? nullptr : Soft.LoadSynchronous();
}

UAnimMontage* ResolveFromCache(const FDFComboDirectionalMontageCache& Cache, const EDFDodgeDirection Dir)
{
	switch (Dir)
	{
	case EDFDodgeDirection::Forward:
		return Cache.Forward;
	case EDFDodgeDirection::ForwardRight:
		return Cache.ForwardRight;
	case EDFDodgeDirection::Right:
		return Cache.Right;
	case EDFDodgeDirection::BackwardRight:
		return Cache.BackwardRight;
	case EDFDodgeDirection::Backward:
		return Cache.Backward;
	case EDFDodgeDirection::BackwardLeft:
		return Cache.BackwardLeft;
	case EDFDodgeDirection::Left:
		return Cache.Left;
	case EDFDodgeDirection::ForwardLeft:
		return Cache.ForwardLeft;
	default:
		return nullptr;
	}
}
} // namespace

bool FDFComboDirectionalMontageSet::IsConfigured() const
{
	return !Forward.IsNull() || !ForwardRight.IsNull() || !Right.IsNull() || !BackwardRight.IsNull()
		|| !Backward.IsNull() || !BackwardLeft.IsNull() || !Left.IsNull() || !ForwardLeft.IsNull();
}

UAnimMontage* FDFComboDirectionalMontageSet::ResolveSoft(const EDFDodgeDirection Dir) const
{
	return DFResolveComboDirectionalMontage(*this, Dir);
}

bool FDFComboDirectionalMontageCache::IsConfigured() const
{
	return Forward || ForwardRight || Right || BackwardRight || Backward || BackwardLeft || Left || ForwardLeft;
}

UAnimMontage* FDFComboDirectionalMontageCache::ResolveWithFallback(const EDFDodgeDirection Dir) const
{
	TArray<EDFDodgeDirection> Order;
	DFGetDodgeDirectionResolveOrder(Dir, Order);
	for (const EDFDodgeDirection Candidate : Order)
	{
		if (UAnimMontage* const M = ResolveFromCache(*this, Candidate))
		{
			return M;
		}
	}
	return nullptr;
}

UAnimMontage* DFResolveComboDirectionalMontage(
	const FDFComboDirectionalMontageSet& SoftSet, const EDFDodgeDirection Dir)
{
	FDFComboDirectionalMontageCache Temp;
	DFBuildComboDirectionalCache(SoftSet, Temp);
	return Temp.ResolveWithFallback(Dir);
}

void DFBuildComboDirectionalCache(
	const FDFComboDirectionalMontageSet& SoftSet, FDFComboDirectionalMontageCache& OutCache)
{
	OutCache.Forward = ResolveSoftMontage(SoftSet.Forward);
	OutCache.ForwardRight = ResolveSoftMontage(SoftSet.ForwardRight);
	OutCache.Right = ResolveSoftMontage(SoftSet.Right);
	OutCache.BackwardRight = ResolveSoftMontage(SoftSet.BackwardRight);
	OutCache.Backward = ResolveSoftMontage(SoftSet.Backward);
	OutCache.BackwardLeft = ResolveSoftMontage(SoftSet.BackwardLeft);
	OutCache.Left = ResolveSoftMontage(SoftSet.Left);
	OutCache.ForwardLeft = ResolveSoftMontage(SoftSet.ForwardLeft);
}
