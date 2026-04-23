// Source/DungeonForged/Public/Interaction/ADFInteractableBase.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/DFInteractable.h"
#include "ADFInteractableBase.generated.h"

class ACharacter;
class UStaticMeshComponent;
class UWidgetComponent;
class UPrimitiveComponent;
class USphereComponent;
class UDFInteractionPromptWidget;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFInteractableBase : public AActor, public IDFInteractable
{
	GENERATED_BODY()

public:
	ADFInteractableBase();

	virtual FText GetInteractionText_Implementation() const override;
	virtual bool CanInteract_Implementation(ACharacter* Interactor) const override;
	virtual float GetInteractionRange_Implementation() const override;
	virtual void Interact_Implementation(ACharacter* Interactor) override;

	/** Called on successful interact on authority; play feedback then mark single-use. */
	UFUNCTION(BlueprintNativeEvent, Category = "DF|Interaction")
	void PlayInteractEffects(ACharacter* Interactor);
	virtual void PlayInteractEffects_Implementation(ACharacter* Interactor);

	void SetPromptPrimaryFocus(bool bIsPrimary) const;
	void SetPromptWidgetVisible(const bool bVisible) const;
	void SetPromptData(const FText& Action, UTexture2D* KeyIcon) const;
	void RefreshPrompt() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Interaction")
	TObjectPtr<USphereComponent> InteractionRange = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Interaction")
	TObjectPtr<UStaticMeshComponent> Mesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Interaction|UI", meta = (DisplayName = "Interaction Prompt (WBP)"))
	TObjectPtr<UWidgetComponent> InteractionPromptWidget = nullptr;

	/** C++: parent class of WBP_InteractionPrompt. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Interaction|UI")
	TSubclassOf<UDFInteractionPromptWidget> InteractionPromptClass;

	/** Key icon (optional) for [G] style prompts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Interaction|UI")
	TObjectPtr<UTexture2D> DefaultKeyIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "DF|Interaction")
	bool bIsInteractable = true;

	/** If true, the first successful interact disables further use. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "DF|Interaction")
	bool bSingleUse = true;

protected:
	UFUNCTION()
	void OnRangeBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnRangeEndOverlap(
		UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void SyncInteractionSphereRadius() const;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Tweak in subclasses for animation length before hiding single-use. */
	UPROPERTY(EditAnywhere, Category = "DF|Interaction")
	float InteractSettleTime = 0.35f;

	FTimerHandle SingleUseTimer;
};
