// Source/DungeonForged/Public/GAS/Abilities/Equipment/UDFAbility_StowWeapon.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "Equipment/DFEquipmentTypes.h"
#include "UDFAbility_StowWeapon.generated.h"

struct FGameplayAbilityActorInfo;

/** Plays optional stash montage then RequestUnequipSlot (weapon by default). */
UCLASS()
class DUNGEONFORGED_API UDFAbility_StowWeapon : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_StowWeapon();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	EEquipmentSlot StowSlot = EEquipmentSlot::Weapon;

protected:
	virtual void PostInitProperties() override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnMontageCompletedOrCancelled();

	void RequestUnequipFromAnimation();
};
