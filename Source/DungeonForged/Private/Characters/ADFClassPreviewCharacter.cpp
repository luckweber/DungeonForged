// Source/DungeonForged/Private/Characters/ADFClassPreviewCharacter.cpp
#include "Characters/ADFClassPreviewCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

ADFClassPreviewCharacter::ADFClassPreviewCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::Disabled;

	if (UCharacterMovementComponent* const Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
	}
}

void ADFClassPreviewCharacter::BeginPlay()
{
	Super::BeginPlay();
	SetCanBeDamaged(false);

	if (UCharacterMovementComponent* const Move = GetCharacterMovement())
	{
		Move->Deactivate();
	}
}
