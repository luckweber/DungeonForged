// Source/DungeonForged/Public/GAS/Abilities/Boss/UDFBossAbility_VoidBarrier.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFBossAbility_VoidBarrier.generated.h"

class ADFVoidOrbActor;
class UGameplayEffect;
class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class DUNGEONFORGED_API UDFBossAbility_VoidBarrier : public UDFGameplayAbility
{
	GENERATED_BODY()
public:
	UDFBossAbility_VoidBarrier();
	virtual void PostInitProperties() override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

	/** All orbs that die while still bound count toward 4. */
	void NotifyVoidOrbDestroyed(ADFVoidOrbActor* Orb);

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnDurationExpired();

	void FinishBarrierFromTimeout();
	void DestroyOrbsSilently();
	int32 OrbsKilled = 0;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Void")
	TSubclassOf<UGameplayEffect> BarrierClass;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Void")
	TSubclassOf<UGameplayEffect> CooldownClass;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Void", meta = (ClampMin = "0"))
	float BarrierDuration = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Void")
	TSubclassOf<ADFVoidOrbActor> OrbClass;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Void", meta = (ClampMin = "1"))
	float OrbitRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Void")
	TObjectPtr<UNiagaraSystem> BarrierVfx;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Void", meta = (ClampMin = "0"))
	float BossStunOnBreakSeconds = 1.f;

	FActiveGameplayEffectHandle ActiveBarrier;
	TArray<TObjectPtr<ADFVoidOrbActor>> Orbs;
	bool bBarrierEnded = false;
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> SpawnedBarrierNiagara;
};
