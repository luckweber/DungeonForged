#pragma once
#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Universal_CallLightning.generated.h"

class UGameplayEffect;
class UNiagaraSystem;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Universal_CallLightning : public UDFGameplayAbility
{
	GENERATED_BODY()
public:
	UDFAbility_Universal_CallLightning();

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Universal")
	TSubclassOf<UGameplayEffect> LightningDamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Universal", meta = (ClampMin = "100.0"))
	float StrikeRange = 900.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Universal", meta = (ClampMin = "50.0"))
	float StrikeRadius = 280.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Universal")
	float BonusBaseDamage = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Universal")
	TObjectPtr<UNiagaraSystem> StrikeVFX = nullptr;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void PostInitProperties() override;

	void StrikeAtLocation(const FVector& Loc);
};
