// Source/DungeonForged/Public/Performance/UDFEnemySignificanceComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDFEnemySignificanceComponent.generated.h"

/**
 * Distance-based anim/VFX budget for enemy crowds (Significance-lite, no engine plugin).
 */
UCLASS(ClassGroup = (Performance), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFEnemySignificanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFEnemySignificanceComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Significance", meta = (ClampMin = "100"))
	float HighDetailDistance = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Significance", meta = (ClampMin = "100"))
	float MediumDetailDistance = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Significance", meta = (ClampMin = "100"))
	float LowDetailDistance = 4500.f;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	void ApplySignificanceTier(int32 Tier) const;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> CachedMesh;

	int32 CurrentTier = 0;
};
