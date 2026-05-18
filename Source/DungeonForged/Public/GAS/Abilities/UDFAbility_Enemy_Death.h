// Source/DungeonForged/Public/GAS/Abilities/UDFAbility_Enemy_Death.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/UDFAbility_Death.h"
#include "UDFAbility_Enemy_Death.generated.h"

UCLASS()
class DUNGEONFORGED_API UUDFAbility_Enemy_Death : public UUDFAbility_Death
{
	GENERATED_BODY()

public:
	UUDFAbility_Enemy_Death();

	/** Delay after montage before Destroy (ElderLore @c UElderGA_Death::EnemyDestroyDelay). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ability|DF|Death")
	float EnemyDestroyDelay = 2.5f;

protected:
	virtual void PostInitProperties() override;
	virtual UAnimMontage* ResolveDeathMontage() const override;
	virtual void ApplyDeathState() override;
	virtual void OnDeathFlowStarted() override;
	virtual void OnDeathMontagePipelineFinished(bool bInterrupted) override;

	virtual void OnDeathGameplayEvent(FGameplayEventData Payload) override;
};
