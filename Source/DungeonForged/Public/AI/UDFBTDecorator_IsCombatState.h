// Source/DungeonForged/Public/AI/UDFBTDecorator_IsCombatState.h
#pragma once

#include "CoreMinimal.h"
#include "AI/DFAIKeys.h"
#include "BehaviorTree/BTDecorator.h"
#include "UDFBTDecorator_IsCombatState.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFBTDecorator_IsCombatState : public UBTDecorator
{
	GENERATED_BODY()

public:
	UDFBTDecorator_IsCombatState();

	UPROPERTY(EditAnywhere, Category = "DF|AI")
	EADFAICombatState RequiredState = EADFAICombatState::Patrol;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
