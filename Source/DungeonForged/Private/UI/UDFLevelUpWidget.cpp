// Source/DungeonForged/Private/UI/UDFLevelUpWidget.cpp

#include "UI/UDFLevelUpWidget.h"
#include "Characters/ADFPlayerState.h"
#include "Progression/UDFLevelingComponent.h"

void UDFLevelUpWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (ADFPlayerState* const PS = GetDFPlayerState())
	{
		if (UDFLevelingComponent* const Lc = PS->GetLevelingComponent())
		{
			CachedLeveling = Lc;
			Lc->OnLevelUp.AddUObject(this, &UDFLevelUpWidget::HandleLevelUpFromComponent);
		}
	}
}

void UDFLevelUpWidget::NativeDestruct()
{
	if (UDFLevelingComponent* const Lc = CachedLeveling.Get())
	{
		Lc->OnLevelUp.RemoveAll(this);
	}
	CachedLeveling.Reset();
	Super::NativeDestruct();
}

void UDFLevelUpWidget::HandleLevelUpFromComponent(int32 const NewLevel, int32 const UnspentAttributePoints)
{
	BP_OnLevelUp(NewLevel, UnspentAttributePoints);
}
