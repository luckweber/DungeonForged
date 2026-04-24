// Source/DungeonForged/Public/Dungeon/Traps/ADFPoisonCloudActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ADFPoisonCloudActor.generated.h"

class USphereComponent;
class UNiagaraComponent;
class ADFTrapBase;

/** Area poison cloud spawned by a vent; lifespan matches CloudDuration. */
UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFPoisonCloudActor : public AActor
{
	GENERATED_BODY()

public:
	ADFPoisonCloudActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Traps|Poison")
	TObjectPtr<USphereComponent> CloudVolume = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Traps|Poison|VFX")
	TObjectPtr<UNiagaraComponent> CloudNiagara = nullptr;

	/** Pushes DoT (SetByCaller) strength; duration comes from the GE or OptionalDuration. */
	UPROPERTY(BlueprintReadWrite, Category = "DF|Traps|Poison|GAS")
	TWeakObjectPtr<ADFTrapBase> SourceTrap;

	/** Ticks poison application. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Poison", meta = (ClampMin = "0.1"))
	float TickInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Poison|GAS", meta = (ClampMin = "0.0"))
	float DoTTickStrength = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Poison|GAS", meta = (ClampMin = "0.0"))
	float DoTDurationSeconds = 3.f;

	/** If true, local client may drive post-process; hook in Blueprint. */
	UPROPERTY(BlueprintReadWrite, Transient, Category = "DF|Traps|Poison|FX")
	bool bClientIsInCloud = false;

	UFUNCTION(BlueprintImplementableEvent, Category = "DF|Traps|Poison|FX")
	void OnClientCloudVisualChanged(bool bInside);

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void Tick(float DeltaSeconds) override;
	void TickClientCloudUi() const;
	void OnPoisonTick();

	FTimerHandle PoisonTickHandle;
};
