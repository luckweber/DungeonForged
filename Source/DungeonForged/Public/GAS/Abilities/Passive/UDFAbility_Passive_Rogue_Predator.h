// Source/DungeonForged/Public/GAS/Abilities/Passive/UDFAbility_Passive_Rogue_Predator.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/Passive/UDFAbilityPassive.h"
#include "GameplayEffectTypes.h"
#include "UDFAbility_Passive_Rogue_Predator.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFAbility_Passive_Rogue_Predator : public UDFAbilityPassive
{
	GENERATED_BODY()

public:
	UDFAbility_Passive_Rogue_Predator();

	virtual void PostInitProperties() override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

protected:
	virtual void OnPassiveAbilityActivated(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo& ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	FActiveGameplayEffectHandle AuraGEHandle;
};
