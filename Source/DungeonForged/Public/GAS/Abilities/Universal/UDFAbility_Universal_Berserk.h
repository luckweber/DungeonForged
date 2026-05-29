#pragma once
#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Universal_Berserk.generated.h"

class UGameplayEffect;
struct FActiveGameplayEffectHandle;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Universal_Berserk : public UDFGameplayAbility
{
	GENERATED_BODY()
public:
	UDFAbility_Universal_Berserk();

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Universal")
	TSubclassOf<UGameplayEffect> BerserkBuffClass;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void PostInitProperties() override;

	void SetBerserkPresentation(bool bActive) const;

	FActiveGameplayEffectHandle ActiveBuffHandle;
};
