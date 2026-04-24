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
#include "World/UDFWorldTransitionSubsystem.h"
#include "Interaction/UDFInteractionComponent.h"
#include "Blueprint/UserWidget.h"

ADFNexusPlayerController::ADFNexusPlayerController()
{
	bShowMouseCursor = true;
}

void ADFNexusPlayerController::OnPossess(APawn* const InPawn)
{
	Super::OnPossess(InPawn);
	if (IsLocalController())
	{
		FInputModeGameAndUI M;
		M.SetHideCursorDuringCapture(false);
		M.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(M);
		SetShowMouseCursor(true);
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
	if (ClassSelectionInstance)
	{
		return;
	}
	const TSubclassOf<UDFNexusClassSelectionWidget> C = ClassSelectionClass
			? ClassSelectionClass
			: TSubclassOf<UDFNexusClassSelectionWidget>(UDFNexusClassSelectionWidget::StaticClass());
	if (!C)
	{
		return;
	}
	UDFNexusClassSelectionWidget* const Wgt = CreateWidget<UDFNexusClassSelectionWidget>(this, C);
	if (!Wgt)
	{
		return;
	}
	ClassSelectionInstance = Wgt;
	Wgt->AddToViewport(100);
	FInputModeUIOnly Mo;
	Mo.SetWidgetToFocus(Wgt->TakeWidget());
	SetInputMode(Mo);
	SetShowMouseCursor(true);
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
