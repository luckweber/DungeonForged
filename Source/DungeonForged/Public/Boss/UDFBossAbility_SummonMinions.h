// Source/DungeonForged/Public/Boss/UDFBossAbility_SummonMinions.h
#pragma once

#include "CoreMinimal.h"
#include "Boss/UDFBossMinionComponent.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFBossAbility_SummonMinions.generated.h"

struct FGameplayAbilitySpecHandle;
struct FGameplayAbilityActorInfo;
struct FGameplayAbilityActivationInfo;
struct FGameplayEventData;
class ADFEnemyBase;
class UDataTable;

UCLASS()
class DUNGEONFORGED_API UDFBossAbility_SummonMinions : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFBossAbility_SummonMinions();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void PostInitProperties() override;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Summon", meta = (ClampMin = "1"))
	int32 MaxLivingMinions = 6;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Summon")
	TSubclassOf<ADFEnemyBase> MinionClass;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Summon")
	TArray<FName> SpawnSocketNames = {FName("hand_l"), FName("hand_r"), FName("Root")};

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Summon")
	TObjectPtr<UDataTable> MinionDataTable;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Summon")
	FName MinionRowName = NAME_None;

	/** Role per spawn index; cycles when the array is shorter than socket count. */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Summon")
	TArray<EDFBossMinionRole> MinionSpawnRoles = {
		EDFBossMinionRole::Guard,
		EDFBossMinionRole::Exploder,
		EDFBossMinionRole::Guard,
		EDFBossMinionRole::Guard,
		EDFBossMinionRole::Exploder,
		EDFBossMinionRole::Guard};

	UFUNCTION()
	void OnSummonMontageCompleted();

	UFUNCTION()
	void OnSummonMontageInterrupted();
};
