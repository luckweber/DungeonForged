// Source/DungeonForged/Public/GAS/Abilities/UDFAbility_Player_Death.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/UDFAbility_Death.h"
#include "UDFAbility_Player_Death.generated.h"

UCLASS()
class DUNGEONFORGED_API UUDFAbility_Player_Death : public UUDFAbility_Death
{
	GENERATED_BODY()

public:
	UUDFAbility_Player_Death();

protected:
	virtual void PostInitProperties() override;
	virtual UAnimMontage* ResolveDeathMontage() const override;
	virtual void ApplyDeathState() override;
	virtual void OnDeathFlowStarted() override;
	virtual void OnDeathMontagePipelineFinished(bool bInterrupted) override;
};
