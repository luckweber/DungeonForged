// Source/DungeonForged/Public/AI/UDFBTDecorator_IsArchetype.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "Data/DFDataTableStructs.h"
#include "UDFBTDecorator_IsArchetype.generated.h"

/** True when the controlled enemy's archetype matches (Patch 6 dispatch). */
UCLASS()
class DUNGEONFORGED_API UDFBTDecorator_IsArchetype : public UBTDecorator
{
	GENERATED_BODY()

public:
	UDFBTDecorator_IsArchetype();

	UPROPERTY(EditAnywhere, Category = "DF|AI")
	EDFEnemyArchetype Archetype = EDFEnemyArchetype::Grunt;

protected:
	virtual bool CalculateRawConditionValue(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
