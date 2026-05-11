// Source/DungeonForged/Public/GAS/Abilities/Equipment/UDFAbility_EquipmentWeaponToggle.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "Equipment/DFEquipmentTypes.h"
#include "UDFAbility_EquipmentWeaponToggle.generated.h"

struct FGameplayAbilityActorInfo;
class ADFPlayerCharacter;

/**
 * Uma tecla: sem arma equipada pede equip ( WeaponItemRowWhenUnarmed deve estar na bolsa ); com arma, desequipa.
 * Opcionalmente toca montagens diferentes de sacar ou guardar; se null, faz o RPC direto ao fim da ativação.
 */
UCLASS()
class DUNGEONFORGED_API UDFAbility_EquipmentWeaponToggle : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_EquipmentWeaponToggle();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	FName WeaponItemRowWhenUnarmed = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	EEquipmentSlot WeaponSlot = EEquipmentSlot::Weapon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Animation")
	TObjectPtr<UAnimMontage> DrawWeaponMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Animation")
	TObjectPtr<UAnimMontage> StowWeaponMontage;

protected:
	virtual void PostInitProperties() override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnMontageCompletedOrCancelled();

	void ApplyEquipOrUnequipRequest();
	UAnimMontage* ResolveMontageForCurrentState(class ADFPlayerCharacter* PC, bool& OutShouldDraw) const;
};
