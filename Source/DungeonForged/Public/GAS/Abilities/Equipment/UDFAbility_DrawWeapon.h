// Source/DungeonForged/Public/GAS/Abilities/Equipment/UDFAbility_DrawWeapon.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "Equipment/DFEquipmentTypes.h"
#include "UDFAbility_DrawWeapon.generated.h"

struct FGameplayAbilityActorInfo;

/**
 * Plays optional AbilityMontage, then RPCs equip (typically Weapon slot).
 * Set WeaponItemRowToEquip + montage (draw sword) on the BP/CDO; item must exist in inventory.
 */
UCLASS()
class DUNGEONFORGED_API UDFAbility_DrawWeapon : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_DrawWeapon();

	/** Row name in DT_Items; must exist in UDFInventoryComponent on the pawn. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	FName WeaponItemRowToEquip = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	EEquipmentSlot EquipWeaponSlot = EEquipmentSlot::Weapon;

protected:
	virtual void PostInitProperties() override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnMontageCompletedOrCancelled();

	void RequestEquipFromAnimation();
};
