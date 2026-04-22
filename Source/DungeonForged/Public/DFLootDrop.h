// Source/DungeonForged/Public/DFLootDrop.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "Data/DFDataTableStructs.h"
#include "DFLootDrop.generated.h"

class UDataTable;
class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class USphereComponent;
class UStaticMeshComponent;
class USoundBase;
class UNiagaraSystem;

USTRUCT(BlueprintType)
struct FDFLootReplicated
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "DF|Loot")
	FName ItemRowName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "DF|Loot")
	FSoftObjectPath ItemDataTablePath;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDFLootPickedUp, AActor*, InteractingActor, FName, ItemRowName);

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFLootDrop : public AActor
{
	GENERATED_BODY()

public:
	ADFLootDrop();

	/** Set mesh, emissive, physics impulse, collision after spawn (called by loot generator or spawn code). */
	UFUNCTION(BlueprintCallable, Category = "DF|Loot")
	void InitLoot(
		UDataTable* InItemDataTable, FName InItemRowName, FVector WorldImpulse, bool bInRandomizeImpulse = true);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Loot")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Loot")
	TObjectPtr<USphereComponent> PickupSphere;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Loot, Category = "DF|Loot")
	FDFLootReplicated Loot;

	/** Server-side cache; clients resolve via ItemDataTablePath. */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "DF|Loot")
	TObjectPtr<UDataTable> ItemDataTable = nullptr;

	/** If set, a MID is created and this vector (emissive) parameter receives rarity color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot|VFX")
	FName RarityColorParameterName = FName(TEXT("RarityColor"));

	/** If no dynamic override: applied as vector parameter on a newly created dynamic instance of this material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot|VFX")
	TObjectPtr<UMaterialInterface> RarityBaseMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot|FX")
	TObjectPtr<UNiagaraSystem> PickupNiagaraVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot|Audio")
	TObjectPtr<USoundBase> PickupSound = nullptr;

	UPROPERTY(BlueprintAssignable, Category = "DF|Loot|Events")
	FOnDFLootPickedUp OnPickedUp;

	void PlayPickupLocalFeedback();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayPickupVFX(AActor* InteractingActor, FVector VFXLocation);

	FName GetItemRowName() const { return Loot.ItemRowName; }

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnPickupSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Random impulse direction (horizontal bias) and magnitude. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot|Physics", meta = (ClampMin = "0.0"))
	float DropImpulseMin = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot|Physics", meta = (ClampMin = "0.0"))
	float DropImpulseMax = 400.f;

	UPROPERTY(Transient, DuplicateTransient)
	TObjectPtr<UMaterialInstanceDynamic> RarityMaterialMID = nullptr;

	UFUNCTION()
	void OnRep_Loot();

	void ApplyRarityEmissive();
	void ApplyVisualsFromDataTable();
	static FLinearColor RarityToLinearColor(EItemRarity Rarity);

	bool bInitialized = false;
};
