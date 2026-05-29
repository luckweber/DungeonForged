#pragma once
#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Universal_Siphon.generated.h"

struct FGameplayEventData;
class UGameplayEffect;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Universal_Siphon : public UDFGameplayAbility
{
	GENERATED_BODY()
public:
	UDFAbility_Universal_Siphon();

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Universal")
	TSubclassOf<UGameplayEffect> TrueDamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Universal", meta = (ClampMin = "50.0"))
	float TraceRange = 350.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Universal")
	float BonusBaseDamage = 45.f;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void PostInitProperties() override;

	UFUNCTION()
	void OnSiphonTraceEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnSiphonFallbackDelay();

	void ApplySiphonHit();
	void FinishSiphon();

	bool bSiphonResolved = false;
};
