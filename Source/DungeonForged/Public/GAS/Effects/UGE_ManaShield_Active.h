// Source/DungeonForged/Public/GAS/Effects/UGE_ManaShield_Active.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_ManaShield_Active.generated.h"

/** Infinite until removed; grants State.ManaShieldActive for damage redirect in UDFAttributeSet. */
UCLASS()
class DUNGEONFORGED_API UGE_ManaShield_Active : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_ManaShield_Active();

protected:
	virtual void ConfigureEffectCDO() override;
};
