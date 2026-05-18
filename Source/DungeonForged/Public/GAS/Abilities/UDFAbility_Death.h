// Source/DungeonForged/Public/GAS/Abilities/UDFAbility_Death.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "GameplayTagContainer.h"
#include "UDFAbility_Death.generated.h"

struct FGameplayEventData;
class UAnimMontage;

/**
 * Shared death flow: apply dead state, play montage (PlayMontageAndWait + optional GameplayEvent),
 * then subclass hook when the montage pipeline finishes.
 */
UCLASS(Abstract)
class DUNGEONFORGED_API UUDFAbility_Death : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UUDFAbility_Death();

protected:
	virtual void PostInitProperties() override;

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	/** Montage from avatar (enemy/player BP) or AbilityMontage on this class. */
	virtual UAnimMontage* ResolveDeathMontage() const;

	virtual void ApplyDeathState();
	virtual void OnDeathFlowStarted();
	virtual void OnDeathMontagePipelineFinished(bool bInterrupted);

	/** Optional notify frame (AN_SendGameplayEvent) e.g. loot spawn. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Death")
	FGameplayTag DeathEventTag;

	UFUNCTION()
	void OnDeathMontageCompleted();
	UFUNCTION()
	void OnDeathMontageInterrupted();
	UFUNCTION()
	void OnDeathMontageCancelled();
	UFUNCTION()
	void OnDeathMontageBlendOut();
	UFUNCTION()
	void OnDeathMontageDelayFinished();
	UFUNCTION()
	virtual void OnDeathGameplayEvent(FGameplayEventData Payload);

	void StartDeathMontagePipeline();
	void FinishDeathMontagePipeline(bool bInterrupted);
	void TryFinishDeathAbility(bool bInterrupted);

	int32 PendingDeathTasks = 0;
	bool bDeathLootEventHandled = false;
	/** OnCompleted + OnBlendOut can fire for the same montage (ElderLore pattern). */
	bool bMontageCompletionHandled = false;

	/** Enemy sets false — blend-out fires instantly when the montage cannot use the ABP slot. */
	bool bFinishDeathOnMontageBlendOut = true;
};
