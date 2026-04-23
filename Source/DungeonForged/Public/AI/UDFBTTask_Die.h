// Source/DungeonForged/Public/AI/UDFBTTask_Die.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UDFBTTask_Die.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFBTTask_Die : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UDFBTTask_Die();
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
