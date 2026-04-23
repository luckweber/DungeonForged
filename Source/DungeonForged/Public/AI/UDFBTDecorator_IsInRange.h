// Source/DungeonForged/Public/AI/UDFBTDecorator_IsInRange.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "UDFBTDecorator_IsInRange.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFBTDecorator_IsInRange : public UBTDecorator
{
	GENERATED_BODY()

public:
	UDFBTDecorator_IsInRange();

	UPROPERTY(EditAnywhere, Category = "DF|AI", meta = (ClampMin = "0.0"))
	float Range = 500.f;

protected:
	virtual bool CalculateRawConditionValue(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
