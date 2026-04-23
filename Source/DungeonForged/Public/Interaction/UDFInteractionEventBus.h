// Source/DungeonForged/Public/Interaction/UDFInteractionEventBus.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "UDFInteractionEventBus.generated.h"

class AActor;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDFWorldGameplayEvent, FGameplayTag, AActor* /* Instigator */);

/** Broadcasts gameplay tags to listeners (e.g. boss death → open BossDoor). */
UCLASS()
class DUNGEONFORGED_API UDFInteractionEventBus : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void DispatchGameplayEvent(FGameplayTag EventTag, AActor* Instigator);

	FOnDFWorldGameplayEvent OnGameplayEvent;
};
