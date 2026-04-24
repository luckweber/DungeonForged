// Source/DungeonForged/Public/Dungeon/Traps/ADFTrap_DartWall.h
#pragma once

#include "CoreMinimal.h"
#include "Dungeon/Traps/ADFTrapBase.h"
#include "ADFTrap_DartWall.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class ADFDartProjectile;
class USoundBase;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFTrap_DartWall : public ADFTrapBase
{
	GENERATED_BODY()

public:
	ADFTrap_DartWall();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Traps|DartWall")
	TObjectPtr<UBoxComponent> TripwireBox = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Traps|DartWall")
	TObjectPtr<UStaticMeshComponent> WallMesh = nullptr;

	/** In wall local space, relative to WallMesh, positions for dart spawns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|DartWall")
	TArray<FVector> DartSpawnOffsets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|DartWall", meta = (ClampMin = "0.0"))
	float DartSpeed = 1800.f;

	/** true = box beam across hall + overlap; false = `FireInterval` auto volley. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|DartWall")
	bool bIsTripwire = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|DartWall", meta = (ClampMin = "0.1"))
	float FireInterval = 3.f;

	/** Darts; damage uses trap `DamageAmount` unless overridden in projectile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|DartWall")
	TSubclassOf<ADFDartProjectile> DartClass;

	/** Fires across +Y of the wall; rotate the actor in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|DartWall", meta = (ClampMin = "1.0"))
	float DartTravelDistance = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|DartWall|Audio")
	TObjectPtr<USoundBase> WallTelegraphClickSound = nullptr;

	virtual void TelegraphActivation_Implementation(AActor* InstigatorActor) override;
	virtual void OnTrapTriggered_Implementation(AActor* InstigatorActor) override;

	void FireAllDarts(AActor* InstigatorActor, bool bPlayVFX = true);
	void TimedFire();

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnTripwireBeginOverlap(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	FTimerHandle TimedVolleyHandle;
};
