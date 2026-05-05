// Source/DungeonForged/Private/GameModes/Nexus/ADFNexusPlayerController.cpp
#include "GameModes/Nexus/ADFNexusPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Character.h"
#include "GameModes/Nexus/UDFNexusClassSelectionWidget.h"
#include "UI/ClassSelection/UDFClassSelectionSubsystem.h"
#include "World/UDFWorldTransitionSubsystem.h"
#include "Interaction/UDFInteractionComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

ADFNexusPlayerController::ADFNexusPlayerController()
{
	bShowMouseCursor = false;
}

void ADFNexusPlayerController::OnPossess(APawn* const InPawn)
{
	Super::OnPossess(InPawn);
	if (IsLocalController())
	{
		// Action controls: camera look + movement; no software cursor stealing delta.
		UGameplayStatics::SetGamePaused(GetWorld(), false);
		SetShowMouseCursor(false);
		SetInputMode(FInputModeGameOnly());
		if (UEnhancedInputLocalPlayerSubsystem* const Sub =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			if (NexusInputMapping)
			{
				Sub->AddMappingContext(NexusInputMapping, 0);
			}
		}
	}
}

void ADFNexusPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	UEnhancedInputComponent* const EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		return;
	}
	if (IA_Move)
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ADFNexusPlayerController::Input_Move);
	}
	if (IA_Look)
	{
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ADFNexusPlayerController::Input_Look);
	}
	if (IA_Interact)
	{
		EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &ADFNexusPlayerController::Input_Interact);
	}
}

void ADFNexusPlayerController::Input_Move(const FInputActionValue& V)
{
	const FVector2D Axis = V.Get<FVector2D>();
	if (ACharacter* const C = GetPawn<ACharacter>())
	{
		const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);
		const FVector F = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X);
		const FVector R = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);
		C->AddMovementInput(F, Axis.Y);
		C->AddMovementInput(R, Axis.X);
	}
}

void ADFNexusPlayerController::Input_Look(const FInputActionValue& V)
{
	const FVector2D Axis = V.Get<FVector2D>();
	if (FMath::IsNearlyZero(Axis.X) && FMath::IsNearlyZero(Axis.Y))
	{
		return;
	}
	FRotator R = GetControlRotation();
	R.Pitch = FMath::Clamp(R.Pitch + Axis.Y, -80.f, 80.f);
	R.Yaw += Axis.X;
	SetControlRotation(R);
}

void ADFNexusPlayerController::Input_Interact()
{
	if (ACharacter* const C = GetPawn<ACharacter>())
	{
		if (UDFInteractionComponent* const I = C->FindComponentByClass<UDFInteractionComponent>())
		{
			I->TryInteract();
		}
	}
}

void ADFNexusPlayerController::Client_OpenClassSelection_Implementation()
{
	if (UWorld* const W = GetWorld())
	{
		if (UDFClassSelectionSubsystem* const Sub = W->GetSubsystem<UDFClassSelectionSubsystem>())
		{
			if (ClassSelectionClass)
			{
				Sub->ClassSelectionWidgetClass = TSubclassOf<UUserWidget>(ClassSelectionClass);
			}
			Sub->OpenClassSelection();
		}
	}
}

void ADFNexusPlayerController::Server_BeginRunWithClass_Implementation(const FName ClassRow)
{
	if (ClassRow.IsNone())
	{
		return;
	}
	UGameInstance* const GI = GetGameInstance();
	UDFWorldTransitionSubsystem* const W = GI ? GI->GetSubsystem<UDFWorldTransitionSubsystem>() : nullptr;
	if (W)
	{
		W->TravelToRun(ClassRow);
	}
}
