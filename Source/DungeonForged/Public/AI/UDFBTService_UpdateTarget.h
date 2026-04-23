// Source/DungeonForged/Public/AI/UDFBTService_UpdateTarget.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "UDFBTService_UpdateTarget.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFBTService_UpdateTarget : public UBTService
{
	GENERATED_BODY()

public:
	UDFBTService_UpdateTarget();
	virtual void TickNode(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** Pawn/character object channel overlap radius (cm). */
	UPROPERTY(EditAnywhere, Category = "DF|AI", meta = (ClampMin = "0.0"))
	float SearchRadius = 2000.f;

	/** If true, uses camera-to-target line trace. */
	UPROPERTY(EditAnywhere, Category = "DF|AI")
	bool bUseLineOfSight = true;
};
