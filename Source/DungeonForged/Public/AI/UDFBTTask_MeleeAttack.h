// Source/DungeonForged/Public/AI/UDFBTTask_MeleeAttack.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "UDFBTTask_MeleeAttack.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFBTTask_MeleeAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UDFBTTask_MeleeAttack();

	virtual uint16 GetInstanceMemorySize() const override;

protected:
	/** e.g. Ability.Attack.Melee — must match a granted ability on the enemy. */
	UPROPERTY(EditAnywhere, Category = "DF|GAS", meta = (Categories = "Ability"))
	FGameplayTag RequiredTag;

	/** Failsafe if the ASC never reports ability end (e.g. passive). */
	UPROPERTY(EditAnywhere, Category = "DF|GAS", meta = (ClampMin = "0.0"))
	float AbilityEndTimeLimit = 3.f;

	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
