// Source/DungeonForged/Public/GameModes/Nexus/ADFNexusCharacter.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ADFNexusCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UDFInteractionComponent;

UCLASS()
class DUNGEONFORGED_API ADFNexusCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ADFNexusCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nexus|Camera")
	TObjectPtr<USpringArmComponent> SpringArm = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nexus|Camera")
	TObjectPtr<UCameraComponent> Camera = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nexus|Interaction")
	TObjectPtr<UDFInteractionComponent> Interaction = nullptr;
};
