// Source/DungeonForged/Public/GAS/DFAbility_Fireball.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "DFAbility_Fireball.generated.h"

struct FGameplayEventData;
class ADFFireballProjectile;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Fireball : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_Fireball();

	// Assign on the ability CDO: CostGameplayEffectClass = GE_Cost_Fireball, CooldownGameplayEffectClass = GE_Cooldown_Fireball (UGameplayAbility base properties).

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Fireball")
	TSubclassOf<ADFFireballProjectile> FireballProjectileClass;

	/** Skeletal socket used for spawn; must exist on the avatar mesh. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Fireball")
	FName MuzzleSocketName = FName("hand_r");

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnFireLaunchEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterruptedOrCancelled();

	virtual void PostInitProperties() override;
};
