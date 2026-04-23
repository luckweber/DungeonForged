// Source/DungeonForged/Public/Interaction/ADFShrine.h
#pragma once

#include "CoreMinimal.h"
#include "Interaction/ADFInteractableBase.h"
#include "Interaction/DFInteractionTypes.h"
#include "ADFShrine.generated.h"

class ACharacter;
class UNiagaraComponent;
class UNiagaraSystem;
class UGameplayEffect;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFShrine : public ADFInteractableBase
{
	GENERATED_BODY()

public:
	ADFShrine();
	virtual void BeginPlay() override;
	virtual FText GetInteractionText_Implementation() const override;
	virtual bool CanInteract_Implementation(ACharacter* Interactor) const override;
	virtual void Interact_Implementation(ACharacter* Interactor) override;

	UFUNCTION(BlueprintCallable, Category = "DF|Shrine")
	void ClearCooldown();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Shrine")
	EDFShrineType ShrineType = EDFShrineType::Healing;

	/** For non-mystery shrines; applied to the interactor's ASC. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Shrine|GAS")
	TSubclassOf<UGameplayEffect> ShrineEffect = nullptr;

	/** 0 = one use and rely on bSingleUse; >0 = cooldown between uses. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Shrine", meta = (ClampMin = "0.0"))
	float CooldownBetweenUses = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Shrine|VFX")
	TObjectPtr<UNiagaraSystem> ActiveVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Shrine|VFX")
	TObjectPtr<UNiagaraSystem> UsedVFX = nullptr;

	/** For Mystery: three possible outcomes; may be fewer than 3. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Shrine|GAS|Mystery")
	TArray<TSubclassOf<UGameplayEffect>> MysteryEffects;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Shrine|VFX")
	TObjectPtr<UNiagaraComponent> VFXComponent = nullptr;

	bool bOnCooldown = false;
	FTimerHandle CooldownTimer;
	void SetCooldown();
	void OnCooldownEnd();
	void ApplyMystery(ACharacter* Interactor);
};
