// Source/DungeonForged/Public/AI/UDFAINoiseLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "FX/UDFCombatFeedbackTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UDFAINoiseLibrary.generated.h"

/** Emits @c UAISense_Hearing events for player combat (wires the configured hearing sense). */
UCLASS()
class DUNGEONFORGED_API UDFAINoiseLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DF|AI|Noise", meta = (WorldContext = "WorldContextObject"))
	static void ReportNoiseAtLocation(
		const UObject* WorldContextObject,
		AActor* Instigator,
		const FVector Location,
		float Loudness = 1.f,
		float MaxRange = 1400.f,
		FName Tag = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "DF|AI|Noise")
	static void ReportCombatHitNoise(
		AActor* Instigator,
		const FVector Location,
		EDFHitFeedbackBand Band,
		bool bWasLethal = false);

	UFUNCTION(BlueprintCallable, Category = "DF|AI|Noise")
	static void ReportAbilityNoise(AActor* Instigator, float Loudness = 1.2f, float MaxRange = 2200.f);
};
