// Source/DungeonForged/Public/GAS/Abilities/Passive/UDFAbility_Passive_Mage_ManaVortex.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/Passive/UDFAbilityPassive.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "UDFAbility_Passive_Mage_ManaVortex.generated.h"

class UNiagaraSystem;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Passive_Mage_ManaVortex : public UDFAbilityPassive
{
	GENERATED_BODY()

public:
	UDFAbility_Passive_Mage_ManaVortex();

	virtual void PostInitProperties() override;

	UPROPERTY(EditDefaultsOnly, Category = "Passive|VFX")
	TObjectPtr<UNiagaraSystem> ManaVortexVFX;

protected:
	virtual void OnPassiveAbilityActivated(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo& ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnAbilityKillEvent(FGameplayEventData EventData);
};
