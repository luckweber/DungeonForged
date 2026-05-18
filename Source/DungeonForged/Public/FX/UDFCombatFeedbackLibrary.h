// Source/DungeonForged/Public/FX/UDFCombatFeedbackLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "FX/UDFCombatFeedbackTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UDFCombatFeedbackLibrary.generated.h"

class AActor;

/** Central dispatch for hit-stop and camera shake bands. */
UCLASS()
class DUNGEONFORGED_API UDFCombatFeedbackLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DF|Combat|Feedback", meta = (WorldContext = "WorldContextObject"))
	static void PlayHitFeedbackForBand(
		UObject* WorldContextObject,
		EDFHitFeedbackBand Band,
		AActor* InstigatorActor = nullptr);

	UFUNCTION(BlueprintCallable, Category = "DF|Combat|Feedback", meta = (WorldContext = "WorldContextObject"))
	static void PlayHitFeedbackFromDamage(
		UObject* WorldContextObject,
		float DamageMagnitude,
		float MaxHealth,
		bool bIsKnockback,
		AActor* InstigatorActor = nullptr);
};
