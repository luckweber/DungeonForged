// Source/DungeonForged/Public/AI/UDFBTTask_BossMinionGuard.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UDFBTTask_BossMinionGuard.generated.h"

/** Moves to @c TargetLocation while @c bBossMinionGuarding is true (set by @c UDFBTService_BossMinionBehavior). */
UCLASS()
class DUNGEONFORGED_API UDFBTTask_BossMinionGuard : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UDFBTTask_BossMinionGuard();

	UPROPERTY(EditAnywhere, Category = "DF|AI", meta = (ClampMin = "50.0"))
	float AcceptanceRadius = 120.f;

protected:
	virtual uint16 GetInstanceMemorySize() const override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
