// Source/DungeonForged/Public/GAS/Abilities/Passive/UDFAbility_Passive_Warrior_Fortitude.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/Passive/UDFAbilityPassive.h"
#include "GameplayEffectTypes.h"
#include "UDFAbility_Passive_Warrior_Fortitude.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFAbility_Passive_Warrior_Fortitude : public UDFAbilityPassive
{
	GENERATED_BODY()

public:
	UDFAbility_Passive_Warrior_Fortitude();

	virtual void PostInitProperties() override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

protected:
	virtual void OnPassiveAbilityActivated(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo& ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	FActiveGameplayEffectHandle AuraGEHandle;
};
