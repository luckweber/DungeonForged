// Source/DungeonForged/Public/AI/UDFBTTask_RangedAttack.h
#pragma once

#include "CoreMinimal.h"
#include "AI/UDFBTTask_MeleeAttack.h"
#include "UDFBTTask_RangedAttack.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFBTTask_RangedAttack : public UDFBTTask_MeleeAttack
{
	GENERATED_BODY()

public:
	UDFBTTask_RangedAttack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
