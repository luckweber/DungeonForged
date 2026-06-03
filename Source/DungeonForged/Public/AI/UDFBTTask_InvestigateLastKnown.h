// Source/DungeonForged/Public/AI/UDFBTTask_InvestigateLastKnown.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UDFBTTask_InvestigateLastKnown.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFBTTask_InvestigateLastKnown : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UDFBTTask_InvestigateLastKnown();

	UPROPERTY(EditAnywhere, Category = "DF|AI", meta = (ClampMin = "50.0"))
	float AcceptanceRadius = 120.f;

	UPROPERTY(EditAnywhere, Category = "DF|AI", meta = (ClampMin = "1.0"))
	float InvestigateTimeoutSeconds = 8.f;

protected:
	virtual uint16 GetInstanceMemorySize() const override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	void FinishInvestigation(UBehaviorTreeComponent& OwnerComp, const EBTNodeResult::Type Result);
};
