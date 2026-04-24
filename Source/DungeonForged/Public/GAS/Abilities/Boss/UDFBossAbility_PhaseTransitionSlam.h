// Source/DungeonForged/Public/GAS/Abilities/Boss/UDFBossAbility_PhaseTransitionSlam.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFBossAbility_PhaseTransitionSlam.generated.h"

class UAnimMontage;
class UCameraShakeBase;
class UNiagaraSystem;

UCLASS()
class DUNGEONFORGED_API UDFBossAbility_PhaseTransitionSlam : public UDFGameplayAbility
{
	GENERATED_BODY()
public:
	UDFBossAbility_PhaseTransitionSlam();
	virtual void PostInitProperties() override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnEruptEvent(FGameplayEventData Data);

	UFUNCTION()
	void OnMontageEnd();

	void DoRoomSlam();
	void RemovePlayerStunLock();

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Slam")
	TObjectPtr<UAnimMontage> PhaseTransitionSlamMontage;

	/** 15% of target max health, true. */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Slam", meta = (ClampMin = "0", ClampMax = "1"))
	float HealthPercentDamage = 0.15f;

	/** 9999u sphere from boss (covers a dungeon room). */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Slam", meta = (ClampMin = "1"))
	float RoomSlamRadius = 9999.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Slam", meta = (ClampMin = "0"))
	float PlayerLockStunSeconds = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Slam")
	TObjectPtr<UNiagaraSystem> RoomShockwaveNiagara;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Slam")
	TSubclassOf<UCameraShakeBase> ExtremeCameraShake;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Slam", meta = (ClampMin = "0"))
	float ShakeDurationScale = 2.f;

	/** 0.3f white flash: EventMagnitude in SendGameplayEvent to player. */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Slam", meta = (ClampMin = "0"))
	float ScreenWhiteFlashDuration = 0.3f;

	bool bEruptFired = false;
};
