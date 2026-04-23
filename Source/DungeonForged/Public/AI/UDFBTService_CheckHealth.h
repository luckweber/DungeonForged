// Source/DungeonForged/Public/AI/UDFBTService_CheckHealth.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "UDFBTService_CheckHealth.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFBTService_CheckHealth : public UBTService
{
	GENERATED_BODY()

public:
	UDFBTService_CheckHealth();
	virtual void TickNode(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "DF|GAS", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FleeHealthFraction = 0.2f;
};
