#pragma once
#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Universal_BattleHymn.generated.h"

class UGameplayEffect;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Universal_BattleHymn : public UDFGameplayAbility
{
	GENERATED_BODY()
public:
	UDFAbility_Universal_BattleHymn();

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Universal")
	TSubclassOf<UGameplayEffect> BattleHymnBuffClass;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void PostInitProperties() override;
};
