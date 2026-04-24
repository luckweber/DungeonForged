// Source/DungeonForged/Public/UI/UDFLevelUpWidget.h
#pragma once

#include "UI/UDFUserWidgetBase.h"
#include "UDFLevelUpWidget.generated.h"

class UDFLevelingComponent;

/**
 * Binds to the owning player's UDFLevelingComponent (on PlayerState) and forwards level-ups to Blueprint.
 * WBP: implement BP_OnLevelUp; optional BindWidget to Text_Name / list rows and confirm to SpendAttributePoint.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class DUNGEONFORGED_API UDFLevelUpWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	/** Forwarded from UDFLevelingComponent::OnLevelUp (replicated or listen server). */
	UFUNCTION(BlueprintImplementableEvent, Category = "DF|UI|Leveling", meta = (DisplayName = "On Level Up"))
	void BP_OnLevelUp(int32 NewLevel, int32 UnspentAttributePoints);

private:
	void HandleLevelUpFromComponent(int32 NewLevel, int32 UnspentAttributePoints);

	TWeakObjectPtr<UDFLevelingComponent> CachedLeveling;
};
