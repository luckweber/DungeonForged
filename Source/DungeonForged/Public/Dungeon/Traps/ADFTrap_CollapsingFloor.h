// Source/DungeonForged/Public/Dungeon/Traps/ADFTrap_CollapsingFloor.h
#pragma once

#include "CoreMinimal.h"
#include "Dungeon/Traps/ADFTrapBase.h"
#include "ADFTrap_CollapsingFloor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USoundBase;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFTrap_CollapsingFloor : public ADFTrapBase
{
	GENERATED_BODY()

public:
	ADFTrap_CollapsingFloor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Traps|Floor")
	TObjectPtr<UBoxComponent> WalkTrigger = nullptr;

	/** Populated in Blueprint with per-tile meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Floor")
	TArray<TObjectPtr<UStaticMeshComponent>> FloorTiles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Floor", meta = (ClampMin = "0.0"))
	float CollapseDelay = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Floor|GAS", meta = (ClampMin = "0.0"))
	float FallDamage = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Floor|Audio")
	TObjectPtr<USoundBase> CrackRumbleSound = nullptr;

	virtual TArray<UPrimitiveComponent*> GetHighlightPrimitives_Implementation() const override;
	virtual void TelegraphActivation_Implementation(AActor* InstigatorActor) override;

protected:
	void BeginPlay() override;
	virtual void OnTrapTriggered_Implementation(AActor* InstigatorActor) override;

	UFUNCTION()
	void OnWalkOverlap(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void RunCollapse(AActor* Instigator);
	FTimerHandle CollapseTimer;
};
