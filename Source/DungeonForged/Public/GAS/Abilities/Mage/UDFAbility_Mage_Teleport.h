// Source/DungeonForged/Public/GAS/Abilities/Mage/UDFAbility_Mage_Teleport.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Mage_Teleport.generated.h"

struct FGameplayEventData;
class UGameplayEffect;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Mage_Teleport : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_Mage_Teleport();

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|Teleport", meta = (ClampMin = "0.0"))
	float BlinkDistance = 700.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|Teleport", meta = (ClampMin = "0.0"))
	float LockOnOffsetBehind = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|Teleport")
	TSubclassOf<UGameplayEffect> TeleportIFrameClass;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|Teleport")
	TSubclassOf<UGameplayEffect> SpellstealDamageClass;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|Teleport")
	TObjectPtr<class UNiagaraSystem> DepartureNiagara = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|Teleport")
	TObjectPtr<UNiagaraSystem> ArrivalNiagara = nullptr;

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	bool ComputeBlinkDestination(const ACharacter* C, FVector& OutDest) const;

	virtual void PostInitProperties() override;
};
