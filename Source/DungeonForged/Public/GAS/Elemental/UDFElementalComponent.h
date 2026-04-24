// Source/DungeonForged/Public/GAS/Elemental/UDFElementalComponent.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Elemental/DFElementalData.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "UDFElementalComponent.generated.h"

class UDataTable;
struct FGameplayEffectSpecHandle;

/**
 * Runtime profile copied from `DT_EnemyElemental` (or set in BP). Usually on `ADFEnemyBase` or similar.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFElementalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFElementalComponent();

	/** `DT_EnemyElemental` row; `None` = skip InitFromTable. */
	UFUNCTION(BlueprintCallable, Category = "DF|Elemental")
	void InitFromTable(UDataTable* Table, FName RowName);

	EDFElementType GetPrimaryElement() const { return AffinityData.PrimaryElement; }
	float GetResistance(EDFElementType VsElement) const;
	const FDFElementalAffinityRow& GetAffinityData() const { return AffinityData; }

	/** Scales the outgoing damage spec, tags element, runs reactions. Call on authority before `Apply` if you pre-build the spec. */
	UFUNCTION(BlueprintCallable, Category = "DF|Elemental")
	void OnElementalHit(UPARAM(Ref) FGameplayEffectSpecHandle& Spec, EDFElementType IncomingElement, AActor* Instigator);

	void SetCurrentReactionTag(FGameplayTag Tag);
	FGameplayTag GetCurrentReactionTag() const { return CurrentReactionTag; }

protected:
	/** Filled from `InitFromTable` or edited on instance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Elemental", meta = (ShowOnlyInnerProperties))
	FDFElementalAffinityRow AffinityData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Elemental", meta = (Categories = "GameplayTag"))
	FGameplayTag CurrentReactionTag;
};
