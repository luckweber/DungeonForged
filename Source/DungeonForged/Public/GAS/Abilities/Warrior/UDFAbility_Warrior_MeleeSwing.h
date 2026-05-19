// Source/DungeonForged/Public/GAS/Abilities/Warrior/UDFAbility_Warrior_MeleeSwing.h
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Warrior_MeleeSwing.generated.h"

struct FGameplayAbilityActorInfo;

/** GAS melee basic: selects montage from combo by CurrentComboStep, otherwise AbilityMontage. */
UCLASS()
class DUNGEONFORGED_API UDFAbility_Warrior_MeleeSwing : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_Warrior_MeleeSwing();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

protected:
	virtual void PostInitProperties() override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnMontageEnd();

	UFUNCTION()
	void OnDirectMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY(Transient)
	bool bPlayedMontageDirect = false;
};
