// Source/DungeonForged/Public/Combat/UDFCombatEventsLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UDFCombatEventsLibrary.generated.h"

class AActor;

USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFDamageDealtContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TObjectPtr<AActor> Source = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TObjectPtr<AActor> Victim = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float Magnitude = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsCrit = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bWasLethal = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayTagContainer Tags;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDFDamageDealt, FDFDamageDealtContext, Context);

/** Global combat event bus (damage dealt, room spectacle hooks). */
UCLASS()
class DUNGEONFORGED_API UDFCombatEventsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static FOnDFDamageDealt& GetOnDamageDealtDelegate();

	UFUNCTION(BlueprintCallable, Category = "DF|Combat|Events")
	static void BroadcastDamageDealt(const FDFDamageDealtContext& Context);

	UFUNCTION(BlueprintCallable, Category = "DF|Combat|Events", meta = (WorldContext = "WorldContextObject"))
	static void NotifyFinisherTargetAvailable(UObject* WorldContextObject, AActor* Attacker, AActor* Victim);
};
