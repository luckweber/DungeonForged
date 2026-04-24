// Source/DungeonForged/Public/GameModes/Nexus/ADFNexusPlayerController.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ADFNexusPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UUserWidget;
class UDFNexusClassSelectionWidget;

UCLASS()
class DUNGEONFORGED_API ADFNexusPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ADFNexusPlayerController();

	/** Lighter set than combat run: Move / Look / Interact. */
	UPROPERTY(EditAnywhere, Category = "Nexus|Input")
	TObjectPtr<UInputMappingContext> NexusInputMapping = nullptr;

	UPROPERTY(EditAnywhere, Category = "Nexus|Input")
	TObjectPtr<UInputAction> IA_Move = nullptr;

	UPROPERTY(EditAnywhere, Category = "Nexus|Input")
	TObjectPtr<UInputAction> IA_Look = nullptr;

	UPROPERTY(EditAnywhere, Category = "Nexus|Input")
	TObjectPtr<UInputAction> IA_Interact = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Nexus|UI")
	TSubclassOf<UDFNexusClassSelectionWidget> ClassSelectionClass;

	UFUNCTION(Client, Reliable)
	void Client_OpenClassSelection();

	UFUNCTION(Server, Reliable)
	void Server_BeginRunWithClass(FName ClassRow);

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;

	void Input_Move(const struct FInputActionValue& V);
	void Input_Look(const struct FInputActionValue& V);
	void Input_Interact();

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ClassSelectionInstance = nullptr;
};
