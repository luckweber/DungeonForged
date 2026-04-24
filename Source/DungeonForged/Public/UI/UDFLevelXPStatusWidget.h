// Source/DungeonForged/Public/UI/UDFLevelXPStatusWidget.h
#pragma once

#include "UI/UDFUserWidgetBase.h"
#include "UDFLevelXPStatusWidget.generated.h"

class UDFLevelingComponent;

/**
 * HUD: subscribe to UDFLevelingComponent XP and level, drive a progress bar or text in WBP.
 * Receives all XP changes; fill ratio is 0..1 (GetXPProgress) until max level.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class DUNGEONFORGED_API UDFLevelXPStatusWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	/** Current XP total, amount needed to break to next, level bar fill, current level. */
	UFUNCTION(BlueprintImplementableEvent, Category = "DF|UI|Leveling", meta = (DisplayName = "On XP Or Level Refresh"))
	void BP_OnXPStateUpdated(int32 CurrentXP, int32 XPToNext, float XPFill01, int32 CurrentLevel);

private:
	void OnXPOrLevelFromComponent(int32 CurrentXP, int32 XPToNext);
	void OnLevelUpFromComponent(int32 NewLevel, int32 UnspentAttributePoints);
	void PushFullState(UDFLevelingComponent* Lc);

	TWeakObjectPtr<UDFLevelingComponent> CachedLeveling;
};
