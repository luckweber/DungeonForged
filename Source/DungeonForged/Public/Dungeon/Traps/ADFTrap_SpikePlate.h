// Source/DungeonForged/Public/Dungeon/Traps/ADFTrap_SpikePlate.h
#pragma once

#include "CoreMinimal.h"
#include "Dungeon/Traps/ADFTrapBase.h"
#include "ADFTrap_SpikePlate.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UCurveFloat;
class UTimelineComponent;
class USoundBase;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFTrap_SpikePlate : public ADFTrapBase
{
	GENERATED_BODY()

public:
	ADFTrap_SpikePlate();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Traps|Spike")
	TObjectPtr<UBoxComponent> TriggerBox = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Traps|Spike")
	TObjectPtr<UStaticMeshComponent> PlateMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Traps|Spike")
	TObjectPtr<UStaticMeshComponent> SpikesMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Spike")
	TObjectPtr<UCurveFloat> SpikeHeightCurve = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Traps|Spike")
	TObjectPtr<UTimelineComponent> SpikeTimeline = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Spike|Damage", meta = (ClampMin = "1.0"))
	FVector HitBoxExtent = FVector(80.f, 80.f, 100.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Spike")
	float SpikesEmergeOffset = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Spike", meta = (ClampMin = "0.01"))
	float SpikesEmergeTime = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Spike", meta = (ClampMin = "0.0"))
	float SpikesHoldTime = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Spike|GAS", meta = (ClampMin = "0.0"))
	float BleedDurationSeconds = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Spike|Audio")
	TObjectPtr<USoundBase> TelegraphClickSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Spike|Audio")
	TObjectPtr<USoundBase> SpikesEmergeSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Spike|Perception", meta = (ClampMin = "0.0"))
	float HiddenViewDistance = 300.f;

	virtual bool CanBeSeen_Implementation() const override;
	virtual TArray<UPrimitiveComponent*> GetHighlightPrimitives_Implementation() const override;
	virtual void TelegraphActivation_Implementation(AActor* InstigatorActor) override;

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnTrapTriggered_Implementation(AActor* InstigatorActor) override;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void ApplySpikeDamage(AActor* InstigatorActor) const;
	void AnimateSpikesToAlpha(float Alpha) const;
	void CompleteSpikeSequence(AActor* InstigatorActor);
	void StartRetractAfterHold(AActor* InstigatorActor);
	void OnRetractTimerFinished(AActor* InstigatorActor);

	FVector SpikesStartRelative = FVector::ZeroVector;
	bool bSpikesStateCached = false;

	FTimerHandle SpikesEmergeEndTimer;
	FTimerHandle SpikesAfterHoldTimer;
	FTimerHandle SpikesRetractEndTimer;
};
