// Source/DungeonForged/Public/GAS/Abilities/Rogue/UDFAbility_Rogue_SmokeScreen.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Rogue_SmokeScreen.generated.h"

class UAnimMontage;
class ADFSmokeBombActor;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Rogue_SmokeScreen : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_Rogue_SmokeScreen();

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Rogue|Smoke")
	TObjectPtr<UAnimMontage> ThrowMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Rogue|Smoke")
	TSubclassOf<ADFSmokeBombActor> SmokeBombClass;

	/** World-space arc distance for spawn (cm). */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Rogue|Smoke", meta = (ClampMin = "0.0"))
	float ThrowDistance = 300.f;

protected:
	virtual void PostInitProperties() override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	UFUNCTION()
	void OnMontageEnd();
};
