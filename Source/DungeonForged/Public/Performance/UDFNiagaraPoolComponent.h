// Source/DungeonForged/Public/Performance/UDFNiagaraPoolComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "UDFNiagaraPoolComponent.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/**
 * Reuses a single UNiagaraComponent: Activate on trigger, Deactivate on complete instead of SpawnSystemAtLocation.
 * For dedicated servers, Niagara is typically a no-op; call sites should still null-check.
 */
UCLASS(ClassGroup = (DF), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFNiagaraPoolComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UDFNiagaraPoolComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|VFX|Pool")
	TObjectPtr<UNiagaraSystem> PooledSystem = nullptr;

	/** Safety cap so runaway one-shots do not sim forever (Niagara: max time before instance quiesces). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|VFX|Pool", meta = (ClampMin = "0.0"))
	float MaxSimTime = 8.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|VFX|Pool")
	TObjectPtr<UNiagaraComponent> PooledNiagara = nullptr;

	virtual void OnRegister() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Reset + start the pooled sim. */
	UFUNCTION(BlueprintCallable, Category = "DF|VFX|Pool")
	void PlayPooledVFX();

	/** Stop and drain; keeps the component for reuse. */
	UFUNCTION(BlueprintCallable, Category = "DF|VFX|Pool")
	void StopPooledVFX();
};
