// Source/DungeonForged/Public/AI/UDFBTDecorator_HasGASTag.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "GameplayTagContainer.h"
#include "UDFBTDecorator_HasGASTag.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFBTDecorator_HasGASTag : public UBTDecorator
{
	GENERATED_BODY()

public:
	UDFBTDecorator_HasGASTag();

	UPROPERTY(EditAnywhere, Category = "DF|GAS", meta = (Categories = "Ability"))
	FGameplayTag RequiredTag;

protected:
	virtual bool CalculateRawConditionValue(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
