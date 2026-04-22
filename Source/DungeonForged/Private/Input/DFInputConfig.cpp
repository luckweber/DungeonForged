// Source/DungeonForged/Private/Input/DFInputConfig.cpp

#include "Input/DFInputConfig.h"

const UInputAction* UDFInputConfig::FindNativeInputActionByTag(const FGameplayTag& Tag) const
{
	if (!Tag.IsValid())
	{
		return nullptr;
	}
	for (const FDFInputAction& Row : NativeInputActions)
	{
		if (Row.InputTag.IsValid() && Row.InputTag == Tag)
		{
			return Row.Action;
		}
	}
	return nullptr;
}
