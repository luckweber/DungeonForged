// Source/DungeonForged/Public/Dungeon/Traps/ADFTrap_PoisonVent.h
#pragma once

#include "CoreMinimal.h"
#include "Dungeon/Traps/ADFTrapBase.h"
#include "ADFTrap_PoisonVent.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class ADFPoisonCloudActor;
class USoundBase;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFTrap_PoisonVent : public ADFTrapBase
{
	GENERATED_BODY()

public:
	ADFTrap_PoisonVent();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Traps|Poison|Sense")
	TObjectPtr<USphereComponent> DetectionRange = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Traps|Poison|Visual")
	TObjectPtr<UStaticMeshComponent> VentMesh = nullptr;

	/** Filled by runtime when spawning. */
	UPROPERTY(BlueprintReadWrite, Transient, Category = "DF|Traps|Poison|Cloud")
	TObjectPtr<ADFPoisonCloudActor> ActiveCloud = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Poison|Cloud", meta = (ClampMin = "0.0"))
	float CloudRadius = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Poison|Cloud", meta = (ClampMin = "0.1"))
	float CloudDuration = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Poison|Flow", meta = (ClampMin = "0.0"))
	float PostCloudRearmBuffer = 4.f;

	/** 500 units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Poison|Flow", meta = (ClampMin = "1.0"))
	float PlayerDetectionRadius = 500.f;

	/** When player enters, wait before the cloud. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Poison|Flow", meta = (ClampMin = "0.0"))
	float PlayerDetectDelay = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Poison|Cloud")
	TSubclassOf<ADFPoisonCloudActor> CloudClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Poison|Audio")
	TObjectPtr<USoundBase> VentActuateSound = nullptr;

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnDetectBegin(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit);

	void OnDetectDelayFired(APawn* PlayerPawn);

	FTimerHandle PlayerDetectHandle;
};
