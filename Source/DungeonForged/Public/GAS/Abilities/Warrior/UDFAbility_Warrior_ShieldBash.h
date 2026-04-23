// Source/DungeonForged/Public/GAS/Abilities/Warrior/UDFAbility_Warrior_ShieldBash.h
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Warrior_ShieldBash.generated.h"

struct FGameplayAbilityActorInfo;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Warrior_ShieldBash : public UDFGameplayAbility
{
	GENERATED_BODY()
public:
	UDFAbility_Warrior_ShieldBash();

	/** If empty, uses AbilityMontage. */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|ShieldBash")
	TObjectPtr<class UAnimMontage> ShieldBashMontage;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|ShieldBash")
	TSubclassOf<class UCameraShakeBase> LightCameraShake;

protected:
	virtual void PostInitProperties() override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	UFUNCTION()
	void OnMontageEnd();
	UFUNCTION()
	void OnTraceGameplayEvent(FGameplayEventData Payload);

	void DoShieldBashBoxTrace() const;
	void PlayLightCameraShake() const;
};
