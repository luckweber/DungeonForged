// Source/DungeonForged/Public/Combat/UDFWeaponTrailPoolComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDFWeaponTrailPoolComponent.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/** Pre-spawned weapon trail Niagara components (B13). Activate on trace start, deactivate on trace end. */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFWeaponTrailPoolComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFWeaponTrailPoolComponent();

	UPROPERTY(EditAnywhere, Category = "DF|FX|Trail")
	TObjectPtr<UNiagaraSystem> TrailSystem = nullptr;

	UPROPERTY(EditAnywhere, Category = "DF|FX|Trail", meta = (ClampMin = "1", ClampMax = "4"))
	int32 PoolSize = 2;

	UPROPERTY(EditAnywhere, Category = "DF|FX|Trail")
	FName AttachSocketName = FName("weapon_end");

	UFUNCTION(BlueprintCallable, Category = "DF|FX|Trail")
	void ActivateTrail();

	UFUNCTION(BlueprintCallable, Category = "DF|FX|Trail")
	void DeactivateTrail();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> Pool;

	int32 ActiveIndex = 0;
	void BuildPool();
};
