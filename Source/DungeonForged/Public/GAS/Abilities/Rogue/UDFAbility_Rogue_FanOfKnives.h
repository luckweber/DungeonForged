// Source/DungeonForged/Public/GAS/Abilities/Rogue/UDFAbility_Rogue_FanOfKnives.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Rogue_FanOfKnives.generated.h"

class UAnimMontage;
class UDFComboPointsComponent;
class ADFKnifeProjectile;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Rogue_FanOfKnives : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_Rogue_FanOfKnives();

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Rogue|Fan")
	TObjectPtr<UAnimMontage> FanMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Rogue|Fan", meta = (ClampMin = "0.0"))
	float FanRadius = 350.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Rogue|Fan")
	TSubclassOf<ADFKnifeProjectile> KnifeClass;

protected:
	virtual void PostInitProperties() override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnMontageEnd();

	UFUNCTION()
	void OnTraceEvent(FGameplayEventData Payload);

	void SpawnKnives();

	static UDFComboPointsComponent* GetCombo(class AActor* From);
};
