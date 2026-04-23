// Source/DungeonForged/Public/AI/UDFBTTask_FleeFromPlayer.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UDFBTTask_FleeFromPlayer.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFBTTask_FleeFromPlayer : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UDFBTTask_FleeFromPlayer();

	/** Accepted radius (cm) for MoveTo. */
	UPROPERTY(EditAnywhere, Category = "DF|AI", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 200.f;

	/** Flee this far in the opposite direction from the player, then let Nav project. */
	UPROPERTY(EditAnywhere, Category = "DF|AI", meta = (ClampMin = "0.0"))
	float FleeSampleDistance = 800.f;

	virtual uint16 GetInstanceMemorySize() const override;
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
