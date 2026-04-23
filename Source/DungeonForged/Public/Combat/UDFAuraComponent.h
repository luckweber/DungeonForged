// Source/DungeonForged/Public/Combat/UDFAuraComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "UDFAuraComponent.generated.h"

class UAbilitySystemComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UGameplayEffect;
class USphereComponent;

/**
 * Overlap-based pulse aura: applies friendly or enemy effects by team (if none from owner, no application).
 * Server authority only for gameplay effect application. Skips re-application when instigator source already has the effect.
 */
UCLASS(Blueprintable, ClassGroup = (DF), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFAuraComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UDFAuraComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura|Shape", meta = (ClampMin = "0.0"))
	float AuraRadius = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura|Tick", meta = (ClampMin = "0.01"))
	float TickInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura|Effects")
	TSubclassOf<UGameplayEffect> FriendlyEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura|Effects")
	TSubclassOf<UGameplayEffect> EnemyEffect;

	/** If set, a looping Niagara is attached to the sphere (optional). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura|VFX")
	TObjectPtr<UNiagaraSystem> LoopingNiagara;

	UFUNCTION(BlueprintCallable, Category = "Aura")
	void SetAuraEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Aura")
	void SetAuraRadius(float NewRadius);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aura|Shape")
	TObjectPtr<USphereComponent> AuraSphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aura|VFX")
	TObjectPtr<UNiagaraComponent> LoopingNiagaraComponent = nullptr;

	FTimerHandle AuraTimer;
	bool bAuraEnabled = true;

	UFUNCTION()
	void PulseAura();

	UAbilitySystemComponent* ResolveOwnerASC() const;
};
