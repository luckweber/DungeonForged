// Source/DungeonForged/Public/AI/UDFBTTask_PlayTauntMontage.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UDFBTTask_PlayTauntMontage.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFBTTask_PlayTauntMontage : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UDFBTTask_PlayTauntMontage();
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
