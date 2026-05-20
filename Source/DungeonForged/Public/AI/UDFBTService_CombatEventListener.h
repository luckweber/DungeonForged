// Source/DungeonForged/Public/AI/UDFBTService_CombatEventListener.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "GameplayTagContainer.h"
#include "UDFBTService_CombatEventListener.generated.h"

class AAIController;
class UAbilitySystemComponent;
struct FGameplayEventData;

/** Listens for parry events on the enemy ASC and sets blackboard recovery state (Patch 2). */
UCLASS()
class DUNGEONFORGED_API UDFBTService_CombatEventListener : public UBTService
{
	GENERATED_BODY()

public:
	UDFBTService_CombatEventListener();

	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "DF|AI", meta = (ClampMin = "0.1"))
	float ParryRecoveryDuration = 1.2f;

protected:
	void OnParryTriggered(const FGameplayEventData* Payload);

	FDelegateHandle ParryDelegateHandle;
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	TWeakObjectPtr<AAIController> BoundAI;
	double RecoverEndWorldTime = 0.0;
};
