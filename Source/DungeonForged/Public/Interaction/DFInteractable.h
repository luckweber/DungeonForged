// Source/DungeonForged/Public/Interaction/DFInteractable.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DFInteractable.generated.h"

class ACharacter;

UINTERFACE(Blueprintable, BlueprintType)
class DUNGEONFORGED_API UDFInteractable : public UInterface
{
	GENERATED_BODY()
};

class DUNGEONFORGED_API IDFInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DF|Interaction")
	bool CanInteract(ACharacter* Interactor) const;
	virtual bool CanInteract_Implementation(ACharacter* Interactor) const { return true; }

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DF|Interaction")
	void Interact(ACharacter* Interactor);
	virtual void Interact_Implementation(ACharacter* Interactor) {}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DF|Interaction")
	FText GetInteractionText() const;
	virtual FText GetInteractionText_Implementation() const { return FText::GetEmpty(); }

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DF|Interaction")
	float GetInteractionRange() const;
	virtual float GetInteractionRange_Implementation() const { return 200.f; }
};
