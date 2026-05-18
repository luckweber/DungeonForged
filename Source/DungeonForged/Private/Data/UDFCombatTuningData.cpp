// Source/DungeonForged/Private/Data/UDFCombatTuningData.cpp
#include "Data/UDFCombatTuningData.h"

FPrimaryAssetId UDFCombatTuningData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CombatTuning"), GetFName());
}
