// Source/DungeonForged/Public/GAS/Elemental/UDFElementalReactionSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Elemental/DFElementalData.h"
#include "Subsystems/WorldSubsystem.h"
#include "UDFElementalReactionSubsystem.generated.h"

class AActor;
class UNiagaraSystem;
class UAbilitySystemComponent;
class UGameplayEffect;
struct FGameplayEffectSpecHandle;

UENUM(BlueprintType)
enum class EDFElementalRuntimeReaction : uint8
{
	None,
	Melt,
	Electrocute,
	Steam,
	TableDriven	UMETA(Hidden)
};

/**
 * Element multipliers, reactions (GAS + VFX), and damage spec massaging. Use on game instance world (not DS for VFX only).
 */
UCLASS()
class DUNGEONFORGED_API UDFElementalReactionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Matrix * row resist, with Arcane/True special cases. */
	UFUNCTION(BlueprintCallable, Category = "DF|Elemental")
	float GetDamageMultiplier(
		EDFElementType AttackElement,
		EDFElementType TargetPrimaryElement,
		const FDFElementalAffinityRow& TargetData) const;

	/**
	 * After incoming element is known: combo reactions (Frost+Fire, etc.), table-driven row tags, Niagara.
	 * Call on authority before/after damage apply depending on your GE design.
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|Elemental")
	EDFElementalRuntimeReaction CheckElementalReaction(AActor* Target, EDFElementType IncomingElement, AActor* Instigator = nullptr);

	/**
	 * Scales `Data.Damage` SetByCaller, adds `Effect.Element.*` to spec dynamic tags, then runs reaction check.
	 * @param Instigator used for effect context; may be null.
	 * @param OptionalTargetRow if null, `UDFElementalComponent` on Target is used when present.
	 */
	void ApplyElementalDamage(
		FGameplayEffectSpecHandle& Spec,
		EDFElementType Element,
		AActor* Instigator,
		AActor* Target,
		const FDFElementalAffinityRow* OptionalTargetRow = nullptr);

	/** VFX for built-in reaction types; author systems under these names. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Elemental|VFX")
	TObjectPtr<UNiagaraSystem> VFX_ReactionMelt = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Elemental|VFX")
	TObjectPtr<UNiagaraSystem> VFX_ReactionElectrocute = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Elemental|VFX")
	TObjectPtr<UNiagaraSystem> VFX_ReactionSteam = nullptr;

	/** Fired for DT_EnemyElemental `ReactionByIncomingElement` only (not Melt/Steam/Electrocute). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Elemental|VFX")
	TObjectPtr<UNiagaraSystem> VFX_ReactionFromTable = nullptr;

	/** Optional: apply when built-in Melt / Electrocute / Steam procs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Elemental|GAS")
	TSubclassOf<UGameplayEffect> GameplayEffect_Melt = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Elemental|GAS")
	TSubclassOf<UGameplayEffect> GameplayEffect_Electrocute = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Elemental|GAS")
	TSubclassOf<UGameplayEffect> GameplayEffect_Steam = nullptr;

	/** Client-side Niagara burst for a reaction; no-op on dedicated server. */
	UFUNCTION(BlueprintCallable, Category = "DF|Elemental|VFX")
	void TrySpawnReactionVFX(EDFElementalRuntimeReaction R, AActor* Target) const;
};
