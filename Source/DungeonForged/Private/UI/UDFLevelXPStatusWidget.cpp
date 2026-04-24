// Source/DungeonForged/Private/UI/UDFLevelXPStatusWidget.cpp

#include "UI/UDFLevelXPStatusWidget.h"
#include "Characters/ADFPlayerState.h"
#include "Progression/UDFLevelingComponent.h"

void UDFLevelXPStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (ADFPlayerState* const PS = GetDFPlayerState())
	{
		if (UDFLevelingComponent* const Lc = PS->GetLevelingComponent())
		{
			CachedLeveling = Lc;
			Lc->OnXPChanged.AddUObject(this, &UDFLevelXPStatusWidget::OnXPOrLevelFromComponent);
			Lc->OnLevelUp.AddUObject(this, &UDFLevelXPStatusWidget::OnLevelUpFromComponent);
			PushFullState(Lc);
		}
	}
}

void UDFLevelXPStatusWidget::NativeDestruct()
{
	if (UDFLevelingComponent* const Lc = CachedLeveling.Get())
	{
		Lc->OnXPChanged.RemoveAll(this);
		Lc->OnLevelUp.RemoveAll(this);
	}
	CachedLeveling.Reset();
	Super::NativeDestruct();
}

void UDFLevelXPStatusWidget::OnXPOrLevelFromComponent(int32 const CurrentXP, int32 const XPToNext)
{
	if (UDFLevelingComponent* const Lc = CachedLeveling.Get())
	{
		BP_OnXPStateUpdated(CurrentXP, XPToNext, Lc->GetXPProgress(), Lc->CurrentLevel);
	}
}

void UDFLevelXPStatusWidget::OnLevelUpFromComponent(int32 const /*NewLevel*/, int32 const /*UnspentAttributePoints*/)
{
	if (UDFLevelingComponent* const Lc = CachedLeveling.Get())
	{
		PushFullState(Lc);
	}
}

void UDFLevelXPStatusWidget::PushFullState(UDFLevelingComponent* const Lc)
{
	if (!Lc)
	{
		return;
	}
	BP_OnXPStateUpdated(Lc->CurrentXP, Lc->GetXPToNextLevel(), Lc->GetXPProgress(), Lc->CurrentLevel);
}
