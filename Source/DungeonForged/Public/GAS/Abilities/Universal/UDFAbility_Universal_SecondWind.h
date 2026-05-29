#pragma once
#include "CoreMinimal.h"
#include "GAS/Abilities/Passive/UDFAbilityPassive.h"
#include "UDFAbility_Universal_SecondWind.generated.h"

/** Passive: grants @c State.Universal.SecondWindAvailable until removed. Lethal save runs in @c UDFAttributeSet. */
UCLASS()
class DUNGEONFORGED_API UDFAbility_Universal_SecondWind : public UDFAbilityPassive
{
	GENERATED_BODY()
public:
	UDFAbility_Universal_SecondWind();
protected:
	virtual void OnPassiveAbilityActivated(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo& ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void PostInitProperties() override;
};
