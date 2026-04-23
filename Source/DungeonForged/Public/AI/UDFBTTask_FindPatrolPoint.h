// Source/DungeonForged/Public/AI/UDFBTTask_FindPatrolPoint.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UDFBTTask_FindPatrolPoint.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFBTTask_FindPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UDFBTTask_FindPatrolPoint();
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
