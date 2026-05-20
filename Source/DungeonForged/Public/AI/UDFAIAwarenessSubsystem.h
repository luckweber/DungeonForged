// Source/DungeonForged/Public/AI/UDFAIAwarenessSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UDFAIAwarenessSubsystem.generated.h"

class AActor;

/** Tracks concurrent enemy telegraphs for BT coordination (Patch 5). */
UCLASS()
class DUNGEONFORGED_API UDFAIAwarenessSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void OnTelegraphBegin(AActor* Enemy);
	void OnTelegraphEnd(AActor* Enemy);

	UFUNCTION(BlueprintPure, Category = "DF|AI|Awareness")
	int32 GetTelegraphingCount() const { return CurrentlyTelegraphing.Num(); }

	UFUNCTION(BlueprintPure, Category = "DF|AI|Awareness")
	int32 GetTelegraphingCountWithin(const FVector& Center, float RadiusCm) const;

	UPROPERTY(EditAnywhere, Category = "DF|AI|Awareness", meta = (ClampMin = "1"))
	int32 MaxConcurrentTelegraphs = 2;

private:
	void PruneInvalid();

	TArray<TWeakObjectPtr<AActor>> CurrentlyTelegraphing;
};
