// Source/DungeonForged/Public/Combat/UDFStyleRatingComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "UDFStyleRatingComponent.generated.h"

UENUM(BlueprintType)
enum class EDFStyleRank : uint8
{
	D,
	C,
	B,
	A,
	S,
	SS,
	SSS
};

USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFStyleEvent
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag MoveTag;

	UPROPERTY()
	float TimeSeconds = 0.f;

	UPROPERTY()
	float ScoreDelta = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStyleRankChanged, EDFStyleRank, NewRank, float, Score);

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFStyleRatingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFStyleRatingComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Style")
	TArray<float> RankThresholds = { 0.f, 50.f, 120.f, 240.f, 400.f, 600.f, 900.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Style", meta = (ClampMin = "0.0"))
	float DecayPerSecond = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Style", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RepeatPenaltyMultiplier = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Style", meta = (ClampMin = "0.5", ClampMax = "10.0"))
	float RepeatPenaltyWindow = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Style")
	bool bDropOnDamage = true;

	UPROPERTY(BlueprintAssignable, Category = "DF|Style")
	FOnStyleRankChanged OnStyleRankChanged;

	UFUNCTION(BlueprintPure, Category = "DF|Style")
	EDFStyleRank GetCurrentRank() const { return CurrentRank; }

	UFUNCTION(BlueprintPure, Category = "DF|Style")
	float GetCurrentScore() const { return Score; }

	UFUNCTION(BlueprintCallable, Category = "DF|Style")
	void RecordMove(FGameplayTag MoveTag, float BaseValue);

	UFUNCTION(BlueprintCallable, Category = "DF|Style")
	void RecordParry() { RecordRaw(40.f); }

	UFUNCTION(BlueprintCallable, Category = "DF|Style")
	void RecordDodgeFlawless() { RecordRaw(25.f); }

	UFUNCTION(BlueprintCallable, Category = "DF|Style")
	void NotifyDamageReceived(float Amount);

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void RecordRaw(float Delta);
	void RecalculateRank();
	void PruneOldEvents(float Now);

	float Score = 0.f;
	EDFStyleRank CurrentRank = EDFStyleRank::D;
	TArray<FDFStyleEvent> RecentEvents;
};
