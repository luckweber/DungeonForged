#pragma once
#include "CoreMinimal.h"
#include "GAS/Effects/UGE_Cooldown_Base.h"
#include "UGE_Cooldown_Universal_HealthPotion.generated.h"

UCLASS()
class DUNGEONFORGED_API UGE_Cooldown_Universal_HealthPotion : public UGE_Cooldown_Base
{
	GENERATED_BODY()
public:
	UGE_Cooldown_Universal_HealthPotion();
protected:
	virtual void ConfigureEffectCDO() override;
};
