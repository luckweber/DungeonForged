// Source/DungeonForged/Public/GAS/Abilities/Passive/UDFAbility_Passive_Rogue_BleedMastery.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/Passive/UDFAbilityPassive.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "UDFAbility_Passive_Rogue_BleedMastery.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFAbility_Passive_Rogue_BleedMastery : public UDFAbilityPassive
{
	GENERATED_BODY()

public:
	UDFAbility_Passive_Rogue_BleedMastery();

	virtual void PostInitProperties() override;

	/** If target has 2+ bleed pressure (stacks or instances), how much armor break to apply (SetByCaller Data.Magnitude, negated in MMC). */
	UPROPERTY(EditDefaultsOnly, Category = "Passive|BleedMastery", meta = (ClampMin = "0.0"))
	float ArmorBreakMagnitude = 10.f;

protected:
	virtual void OnPassiveAbilityActivated(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo& ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnRogueBleedApplied(FGameplayEventData EventData);
};
