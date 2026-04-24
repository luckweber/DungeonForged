// Source/DungeonForged/Public/GAS/Abilities/Boss/UDFBossAbility_MeteorStrike.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFBossAbility_MeteorStrike.generated.h"

class ADFMeteorWarningDecal;
class ADFMeteorImpactActor;
class UGameplayEffect;

UCLASS()
class DUNGEONFORGED_API UDFBossAbility_MeteorStrike : public UDFGameplayAbility
{
	GENERATED_BODY()
public:
	UDFBossAbility_MeteorStrike();
	virtual void PostInitProperties() override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnTelegraphEnd();

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Meteor")
	TSubclassOf<ADFMeteorWarningDecal> WarningDecalClass;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Meteor")
	TSubclassOf<ADFMeteorImpactActor> ImpactClass;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Meteor")
	TSubclassOf<UGameplayEffect> CooldownClass;

	/** 2s from spec. */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Meteor", meta = (ClampMin = "0"))
	float TelegraphTime = 2.f;

	/** 1s prediction along velocity. */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Meteor", meta = (ClampMin = "0"))
	float PlayerLeadSeconds = 1.f;

	/** Filled in Activate; used by telegraph end. */
	FVector PendingMeteorAim = FVector::ZeroVector;
};
