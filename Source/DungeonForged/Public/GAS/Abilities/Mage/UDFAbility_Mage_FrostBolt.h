// Source/DungeonForged/Public/GAS/Abilities/Mage/UDFAbility_Mage_FrostBolt.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Mage_FrostBolt.generated.h"

struct FGameplayEventData;
class ADFFrostBoltProjectile;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Mage_FrostBolt : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_Mage_FrostBolt();

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|FrostBolt")
	TSubclassOf<ADFFrostBoltProjectile> FrostProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|FrostBolt")
	FName MuzzleSocketName = FName("hand_r");

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|FrostBolt")
	TObjectPtr<UAnimMontage> FrostBoltCastMontage;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnFrostTraceEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterruptedOrCancelled();

	virtual void PostInitProperties() override;
};
