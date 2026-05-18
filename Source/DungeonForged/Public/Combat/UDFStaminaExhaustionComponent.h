// Source/DungeonForged/Public/Combat/UDFStaminaExhaustionComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDFStaminaExhaustionComponent.generated.h"

struct FOnAttributeChangeData;
struct FGameplayAttribute;

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFStaminaExhaustionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFStaminaExhaustionComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void OnStaminaChanged(const FOnAttributeChangeData& Data);
	void ClearExhausted();

	FTimerHandle ExhaustTimerHandle;
	FDelegateHandle StaminaDelegateHandle;
};
