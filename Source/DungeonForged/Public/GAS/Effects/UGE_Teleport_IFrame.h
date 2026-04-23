// Source/DungeonForged/Public/GAS/Effects/UGE_Teleport_IFrame.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Teleport_IFrame.generated.h"

/** Short invulnerability after blink. */
UCLASS()
class DUNGEONFORGED_API UGE_Teleport_IFrame : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Teleport_IFrame();

protected:
	virtual void ConfigureEffectCDO() override;
};
