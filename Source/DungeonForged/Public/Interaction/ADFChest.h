// Source/DungeonForged/Public/Interaction/ADFChest.h
#pragma once

#include "CoreMinimal.h"
#include "Interaction/ADFInteractableBase.h"
#include "Data/DFDataTableStructs.h"
#include "ADFChest.generated.h"

class UDataTable;
class UAnimMontage;
class UParticleSystem;
class USkeletalMeshComponent;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFChest : public ADFInteractableBase
{
	GENERATED_BODY()

public:
	ADFChest();

	virtual FText GetInteractionText_Implementation() const override;
	virtual bool CanInteract_Implementation(ACharacter* Interactor) const override;
	virtual void Interact_Implementation(ACharacter* Interactor) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** DataTable of type FDFLootPoolTableRow. Row key = LootTableRow. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot")
	TObjectPtr<UDataTable> LootPoolDataTable = nullptr;

	/** Key into LootPoolDataTable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot")
	FName LootTableRow = NAME_None;

	/** Min item rarity in the pool (floor scaling). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot")
	EItemRarity MinRarity = EItemRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot")
	int32 MinDropCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot")
	int32 MaxDropCount = 4;

	/** In-run gold added when opened (inclusive). 0,0 = no extra gold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot|Gold", meta = (ClampMin = "0"))
	int32 GoldMin = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot|Gold", meta = (ClampMin = "0"))
	int32 GoldMax = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|VFX|Audio")
	TObjectPtr<UAnimMontage> OpenMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|VFX|Audio")
	TObjectPtr<UParticleSystem> OpenVFX = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Visuals")
	TObjectPtr<USkeletalMeshComponent> ChestSkeletalMesh = nullptr;

	/** Replicated: chest already opened. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IsOpen, Category = "DF|State")
	bool bIsOpen = false;

protected:
	UFUNCTION()
	void OnRep_IsOpen();
};
