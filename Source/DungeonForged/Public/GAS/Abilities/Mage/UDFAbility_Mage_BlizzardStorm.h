// Source/DungeonForged/Public/GAS/Abilities/Mage/UDFAbility_Mage_BlizzardStorm.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Mage_BlizzardStorm.generated.h"

struct FGameplayAbilityTargetDataHandle;
class ADFBlizzardZone;
class ADFGroundTargetActor;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Mage_BlizzardStorm : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_Mage_BlizzardStorm();

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|Blizzard")
	TSubclassOf<ADFBlizzardZone> BlizzardZoneClass;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|Blizzard")
	TSubclassOf<ADFGroundTargetActor> GroundTargetActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|Blizzard")
	TObjectPtr<UAnimMontage> BlizzardCastMontage;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnMontageForChannelingDone();

	UFUNCTION()
	void OnMontageInterruptedForBlizzard();

	UFUNCTION()
	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data);

	UFUNCTION()
	void OnTargetCancelled(const FGameplayAbilityTargetDataHandle& Data);

	UFUNCTION()
	void OnBlizzardAbilityEnd();

	virtual void PostInitProperties() override;
};
