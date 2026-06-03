// Source/DungeonForged/Public/AI/UDFBTService_BossMinionBehavior.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "UDFBTService_BossMinionBehavior.generated.h"

/** Updates guard locations and exploder proximity detonation for boss adds. */
UCLASS()
class DUNGEONFORGED_API UDFBTService_BossMinionBehavior : public UBTService
{
	GENERATED_BODY()

public:
	UDFBTService_BossMinionBehavior();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
