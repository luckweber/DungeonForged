// Source/DungeonForged/Public/GameModes/Nexus/ADFNexusNPCBase.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/SphereComponent.h"
#include "Interaction/DFInteractable.h"
#include "ADFNexusNPCBase.generated.h"

class UDataTable;
class UUserWidget;
class UWidgetComponent;
class UDFSaveGame;
class ACharacter;

UCLASS()
class DUNGEONFORGED_API ADFNexusNPCBase : public ACharacter, public IDFInteractable
{
	GENERATED_BODY()

public:
	ADFNexusNPCBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Nexus|NPC")
	FName GetNPCId() const { return NPCId; }

	UFUNCTION(BlueprintCallable, Category = "Nexus|NPC")
	void SetNexusUnlockedFromSave(bool bUnlocked);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nexus|NPC")
	TObjectPtr<USphereComponent> InteractionRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|NPC")
	FName NPCId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|NPC", meta = (MultiLine = "true"))
	FText NPCName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|NPC", meta = (MultiLine = "true"))
	FText NPCDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|NPC")
	TObjectPtr<UAnimMontage> IdleMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|NPC")
	TObjectPtr<UAnimMontage> TalkMontage;

	/** Replicated; save @c UDFSaveGame::UnlockedNPCs also lists @a NPCId. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_NexusUnlocked, Category = "Nexus|NPC")
	bool bIsUnlocked = true;

	/** CDO: Blacksmith + Chronicler start visible before meta rows exist. */
	UPROPERTY(EditAnywhere, Category = "Nexus|NPC")
	bool bDefaultUnlocked = true;

	/** @c FDFNexusUnlockConditionRow in @a UnlockConditionTable (optional). */
	UPROPERTY(EditAnywhere, Category = "Nexus|NPC|Data")
	FName UnlockConditionRow = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Nexus|NPC|Data")
	TObjectPtr<UDataTable> UnlockConditionTable = nullptr;

	UPROPERTY(EditAnywhere, Category = "Nexus|NPC|UI")
	TObjectPtr<UWidgetComponent> NameplateWidget = nullptr;

	UPROPERTY(EditAnywhere, Category = "Nexus|NPC|UI")
	TSubclassOf<UUserWidget> ServiceWidgetClass;

	UFUNCTION(BlueprintCallable, Category = "Nexus|NPC", meta = (CallInEditor = "true"))
	void ApplyUnlockVisual();

	virtual void BeginPlay() override;
	virtual FText GetInteractionText_Implementation() const override;
	virtual bool CanInteract_Implementation(ACharacter* Interactor) const override;
	virtual void Interact_Implementation(ACharacter* Interactor) override;

	UFUNCTION(BlueprintCallable, Category = "Nexus|NPC")
	bool CheckUnlockCondition(UDFSaveGame* Save) const;

	UFUNCTION()
	void OnRep_NexusUnlocked();

protected:
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveServiceWidget = nullptr;

	UFUNCTION()
	void OnNexusRangeBegin(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnNexusRangeEnd(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);
};
