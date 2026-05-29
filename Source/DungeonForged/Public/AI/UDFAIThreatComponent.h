// Source/DungeonForged/Public/AI/UDFAIThreatComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDFAIThreatComponent.generated.h"

/**
 * Per-enemy threat table (server-only). Highest threat within range wins over pure nearest-player.
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFAIThreatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFAIThreatComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|AI|Threat", meta = (ClampMin = "0"))
	float ThreatDecayPerSecond = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|AI|Threat", meta = (ClampMin = "0"))
	float DamageThreatMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|AI|Threat", meta = (ClampMin = "0"))
	float ProximityThreatBias = 0.15f;

	UFUNCTION(BlueprintCallable, Category = "DF|AI|Threat")
	void AddThreat(AActor* Source, float Amount);

	UFUNCTION(BlueprintPure, Category = "DF|AI|Threat")
	AActor* GetBestThreatTarget(
		const FVector Origin,
		float MaxRadius,
		bool bRequireLineOfSight = false,
		AActor* LineOfSightFrom = nullptr) const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(Transient)
	TMap<TWeakObjectPtr<AActor>, float> ThreatByTarget;

	void DecayThreat(float DeltaTime);
};
