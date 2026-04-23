// Source/DungeonForged/Public/Combat/UDFComboPointsComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDFComboPointsComponent.generated.h"

/** WoW-style combo points: builders add, finishers spend (default max 5). */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFComboPointsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFComboPointsComponent();

	/** 0..MaxComboPoints */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Rogue|Combo")
	int32 CurrentComboPoints = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Rogue|Combo", meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxComboPoints = 5;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnComboPointsChanged, int32, NewValue, int32, Max);
	UPROPERTY(BlueprintAssignable, Category = "Combat|Rogue|Combo")
	FOnComboPointsChanged OnComboPointsChanged;

	UFUNCTION(BlueprintCallable, Category = "Combat|Rogue|Combo")
	void AddComboPoints(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Combat|Rogue|Combo")
	bool SpendComboPoints(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Combat|Rogue|Combo")
	void ResetComboPoints();

	UFUNCTION(BlueprintPure, Category = "Combat|Rogue|Combo")
	int32 GetComboPoints() const { return CurrentComboPoints; }

protected:
	void BroadcastIfChanged();
};
