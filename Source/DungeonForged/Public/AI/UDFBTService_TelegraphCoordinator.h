// Source/DungeonForged/Public/AI/UDFBTService_TelegraphCoordinator.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "UDFBTService_TelegraphCoordinator.generated.h"

/** Limits concurrent telegraphs near this pawn (Patch 5). */
UCLASS()
class DUNGEONFORGED_API UDFBTService_TelegraphCoordinator : public UBTService
{
	GENERATED_BODY()

public:
	UDFBTService_TelegraphCoordinator();

	virtual void TickNode(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "DF|AI|Telegraph", meta = (ClampMin = "100.0"))
	float CoordinationRadiusCm = 1200.f;

	UPROPERTY(EditAnywhere, Category = "DF|AI|Telegraph", meta = (ClampMin = "1"))
	int32 MaxConcurrentTelegraphs = 2;
};
