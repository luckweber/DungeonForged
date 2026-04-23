// Source/DungeonForged/Public/AI/ADFAIController.h
#pragma once

#include "CoreMinimal.h"
#include "AI/DFAIKeys.h"
#include "AIController.h"
#include "ADFAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Hearing;
class UAISenseConfig_Sight;
class UBehaviorTree;
class UBehaviorTreeComponent;
class UBlackboardComponent;

/**
 * DungeonForged AI controller: behavior tree + blackboard + perception on the controller.
 * U prefix in spec referred to UObject-style naming; actor classes in this project use ADF.
 */
UCLASS()
class DUNGEONFORGED_API ADFAIController : public AAIController
{
	GENERATED_BODY()

public:
	ADFAIController(const FObjectInitializer& ObjectInitializer);

	//~ AAIController
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/** Blackboard combat state updated for BT and services. */
	UFUNCTION(BlueprintCallable, Category = "DF|AI")
	void SetCombatState(EADFAICombatState State);

	/** Distance to TargetActor blackboard object, or -1 if none. */
	UFUNCTION(BlueprintPure, Category = "DF|AI")
	float GetDistanceToTarget() const;

	/** Forwards to AAIController (populated when the behavior tree is running). */
	UFUNCTION(BlueprintPure, Category = "DF|AI")
	UBlackboardComponent* GetDFBlackboard() const
	{
		return const_cast<UBlackboardComponent*>(GetBlackboardComponent());
	}

	/** Brain after RunBehaviorTree (engine-spawned or cast from BrainComponent). */
	UFUNCTION(BlueprintPure, Category = "DF|AI")
	UBehaviorTreeComponent* GetDFBehaviorTreeComponent() const;

	UFUNCTION(BlueprintPure, Category = "DF|AI")
	UAIPerceptionComponent* GetDFPerception() const { return DFPerception; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|AI")
	TObjectPtr<UAIPerceptionComponent> DFPerception;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|AI")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|AI")
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void OnTargetPerceptionForgotten(AActor* Actor);
};
