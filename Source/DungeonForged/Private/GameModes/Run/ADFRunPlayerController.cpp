// Source/DungeonForged/Private/GameModes/Run/ADFRunPlayerController.cpp
#include "GameModes/Run/ADFRunPlayerController.h"

#include "AbilitySystemComponent.h"
#include "Characters/ADFPlayerCharacter.h"
#include "DungeonForgedModule.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameModes/Run/UDFDefeatScreenWidget.h"
#include "GameModes/Run/UDFVictoryScreenWidget.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "UI/DFAbilityBarTypes.h"
#include "World/DFWorldTypes.h"
#include "World/UDFWorldTransitionSubsystem.h"
#include "Run/UDFBetweenFloorSubsystem.h"
#include "Blueprint/UserWidget.h"

namespace DFRunPlayerControllerInput_Impl
{

static bool IsSameInputActionAsset(const UInputAction* RowAct, const UInputAction* CharAct)
{
	if (!RowAct || !CharAct)
	{
		return false;
	}
	if (RowAct == CharAct)
	{
		return true;
	}
	return RowAct->GetPathName() == CharAct->GetPathName();
}

} // namespace DFRunPlayerControllerInput_Impl

ADFRunPlayerController::ADFRunPlayerController() = default;

ADFPlayerCharacter* ADFRunPlayerController::GetHeroPawn() const
{
	return GetPawn<ADFPlayerCharacter>();
}

UInputMappingContext* ADFRunPlayerController::ResolveGameplayInputMapping() const
{
	if (GameplayInputMapping)
	{
		return GameplayInputMapping;
	}
	return DefaultGameplayIMC;
}

void ADFRunPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		EnsureGameplayInputReady();
	}
}

void ADFRunPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveGameplayMappingContext();
	Super::EndPlay(EndPlayReason);
}

void ADFRunPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (IsLocalController())
	{
		EnsureGameplayInputReady();
	}
}

void ADFRunPlayerController::EnsureGameplayInputReady()
{
	if (!IsLocalController())
	{
		return;
	}
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	SetShowMouseCursor(false);
	SetInputMode(FInputModeGameOnly());
	EnableInput(this);
	AddGameplayMappingContext();
}

void ADFRunPlayerController::AddGameplayMappingContext()
{
	if (bGameplayMappingContextAdded)
	{
		return;
	}
	UInputMappingContext* const IMC = ResolveGameplayInputMapping();
	if (!IMC)
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* const Sub =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Sub->AddMappingContext(IMC, IMC_Priority);
		bGameplayMappingContextAdded = true;
	}
}

void ADFRunPlayerController::RemoveGameplayMappingContext()
{
	if (!bGameplayMappingContextAdded)
	{
		return;
	}
	UInputMappingContext* const IMC = ResolveGameplayInputMapping();
	if (IMC)
	{
		if (UEnhancedInputLocalPlayerSubsystem* const Sub =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Sub->RemoveMappingContext(IMC);
		}
	}
	bGameplayMappingContextAdded = false;
}

void ADFRunPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* const EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		return;
	}

	if (IA_Move)
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ADFRunPlayerController::Input_Move);
	}
	if (IA_Look)
	{
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ADFRunPlayerController::Input_Look);
	}
	if (IA_Jump)
	{
		EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &ADFRunPlayerController::Input_JumpStart);
		EIC->BindAction(IA_Jump, ETriggerEvent::Completed, this, &ADFRunPlayerController::Input_JumpEnd);
	}
	if (IA_CameraZoom)
	{
		EIC->BindAction(IA_CameraZoom, ETriggerEvent::Triggered, this, &ADFRunPlayerController::Input_CameraZoom);
	}
	if (InputConfig)
	{
		RegisterAbilityInputFromConfig(EIC);
	}
	BindAbilityBarSlotInputs(EIC);
	if (IA_SecondaryAttack)
	{
		EIC->BindAction(IA_SecondaryAttack, ETriggerEvent::Started, this, &ADFRunPlayerController::Input_SecondaryAttack);
	}
	if (IA_Sprint)
	{
		EIC->BindAction(IA_Sprint, ETriggerEvent::Started, this, &ADFRunPlayerController::Input_SprintStart);
		EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &ADFRunPlayerController::Input_SprintEnd);
	}
	if (IA_Dodge)
	{
		EIC->BindAction(IA_Dodge, ETriggerEvent::Started, this, &ADFRunPlayerController::Input_Dodge);
	}
	if (IA_LockOn)
	{
		EIC->BindAction(IA_LockOn, ETriggerEvent::Started, this, &ADFRunPlayerController::Input_LockOn);
	}
	if (IA_CycleLockOnLeft)
	{
		EIC->BindAction(IA_CycleLockOnLeft, ETriggerEvent::Started, this, &ADFRunPlayerController::Input_CycleLockOnLeft);
	}
	if (IA_CycleLockOnRight)
	{
		EIC->BindAction(IA_CycleLockOnRight, ETriggerEvent::Started, this, &ADFRunPlayerController::Input_CycleLockOnRight);
	}
	if (IA_EquipmentWeaponToggle)
	{
		EIC->BindAction(
			IA_EquipmentWeaponToggle, ETriggerEvent::Started, this, &ADFRunPlayerController::Input_EquipmentWeaponToggle);
	}
	if (!InputConfig)
	{
		if (IA_Attack)
		{
			EIC->BindAction(IA_Attack, ETriggerEvent::Started, this, &ADFRunPlayerController::Input_AttackPressed);
			EIC->BindAction(IA_Attack, ETriggerEvent::Completed, this, &ADFRunPlayerController::Input_AttackReleased);
		}
		if (IA_Interact)
		{
			EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &ADFRunPlayerController::Input_Interact);
		}
	}
}

void ADFRunPlayerController::RegisterAbilityInputFromConfig(UEnhancedInputComponent* const EIC)
{
	if (!EIC || !InputConfig)
	{
		return;
	}
	for (int32 Index = 0; Index < InputConfig->AbilityInputActions.Num(); ++Index)
	{
		const FDFInputAction& Row = InputConfig->AbilityInputActions[Index];
		if (!Row.Action)
		{
			continue;
		}
		if (IA_Move && DFRunPlayerControllerInput_Impl::IsSameInputActionAsset(Row.Action, IA_Move))
		{
			continue;
		}
		if (IA_Look && DFRunPlayerControllerInput_Impl::IsSameInputActionAsset(Row.Action, IA_Look))
		{
			continue;
		}
		if (IA_Jump && DFRunPlayerControllerInput_Impl::IsSameInputActionAsset(Row.Action, IA_Jump))
		{
			continue;
		}
		if (IA_CameraZoom && DFRunPlayerControllerInput_Impl::IsSameInputActionAsset(Row.Action, IA_CameraZoom))
		{
			continue;
		}
		if (IA_Sprint && DFRunPlayerControllerInput_Impl::IsSameInputActionAsset(Row.Action, IA_Sprint))
		{
			continue;
		}
		if (IA_Dodge && DFRunPlayerControllerInput_Impl::IsSameInputActionAsset(Row.Action, IA_Dodge))
		{
			continue;
		}
		if (IA_Attack && DFRunPlayerControllerInput_Impl::IsSameInputActionAsset(Row.Action, IA_Attack))
		{
			EIC->BindAction(Row.Action, ETriggerEvent::Started, this, &ADFRunPlayerController::Input_AttackPressed);
			EIC->BindAction(Row.Action, ETriggerEvent::Completed, this, &ADFRunPlayerController::Input_AttackReleased);
			continue;
		}
		if (IA_SecondaryAttack
			&& DFRunPlayerControllerInput_Impl::IsSameInputActionAsset(Row.Action, IA_SecondaryAttack))
		{
			EIC->BindAction(Row.Action, ETriggerEvent::Started, this, &ADFRunPlayerController::Input_SecondaryAttack);
			continue;
		}
		if (IA_EquipmentWeaponToggle
			&& DFRunPlayerControllerInput_Impl::IsSameInputActionAsset(Row.Action, IA_EquipmentWeaponToggle))
		{
			EIC->BindAction(
				Row.Action, ETriggerEvent::Started, this, &ADFRunPlayerController::Input_EquipmentWeaponToggle);
			continue;
		}

		int32 InputId = Row.GameplayInputId;
		if (InputId <= 0)
		{
			InputId = Index + 1;
		}
		if (InputId >= 1 && InputId <= DFAbilityBarSlotCount)
		{
			const int32 CapturedSlot = InputId;
			EIC->BindActionValueLambda(Row.Action, ETriggerEvent::Started, [this, CapturedSlot](const FInputActionValue&)
			{
				if (ADFPlayerCharacter* const Hero = GetHeroPawn())
				{
					Hero->TryActivateAbilitySlot(CapturedSlot);
				}
			});
			continue;
		}
		const int32 CapturedId = InputId;
		EIC->BindActionValueLambda(Row.Action, ETriggerEvent::Started, [this, CapturedId](const FInputActionValue&)
		{
			if (ADFPlayerCharacter* const Hero = GetHeroPawn())
			{
				if (UAbilitySystemComponent* const ASC = Hero->GetAbilitySystemComponent())
				{
					ASC->AbilityLocalInputPressed(CapturedId);
				}
			}
		});
		EIC->BindActionValueLambda(Row.Action, ETriggerEvent::Completed, [this, CapturedId](const FInputActionValue&)
		{
			if (ADFPlayerCharacter* const Hero = GetHeroPawn())
			{
				if (UAbilitySystemComponent* const ASC = Hero->GetAbilitySystemComponent())
				{
					ASC->AbilityLocalInputReleased(CapturedId);
				}
			}
		});
	}
}

void ADFRunPlayerController::BindAbilityBarSlotInputs(UEnhancedInputComponent* const EIC)
{
	if (!EIC)
	{
		return;
	}
	auto BindSlot = [this, EIC](UInputAction* const Action, const int32 Slot1Based)
	{
		if (!Action)
		{
			return;
		}
		const int32 Captured = Slot1Based;
		EIC->BindActionValueLambda(Action, ETriggerEvent::Started, [this, Captured](const FInputActionValue&)
		{
			if (ADFPlayerCharacter* const Hero = GetHeroPawn())
			{
				Hero->TryActivateAbilitySlot(Captured);
			}
		});
	};

	if (IA_AbilityBarSlots.Num() > 0)
	{
		const int32 Count = FMath::Min(IA_AbilityBarSlots.Num(), DFAbilityBarSlotCount);
		for (int32 i = 0; i < Count; ++i)
		{
			BindSlot(IA_AbilityBarSlots[i], i + 1);
		}
		return;
	}

	if (!InputConfig)
	{
		BindSlot(IA_Ability1, 1);
		BindSlot(IA_Ability2, 2);
		BindSlot(IA_Ability3, 3);
		BindSlot(IA_Ability4, 4);
	}
}

void ADFRunPlayerController::Input_Move(const FInputActionValue& Value)
{
	ADFPlayerCharacter* const Hero = GetHeroPawn();
	if (!Hero)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Axis.IsNearlyZero())
	{
		return;
	}
	const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
	Hero->AddMovementInput(Forward, Axis.Y);
	Hero->AddMovementInput(Right, Axis.X);
}

void ADFRunPlayerController::Input_Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Axis.IsNearlyZero())
	{
		return;
	}
	FRotator R = GetControlRotation();
	R.Pitch = FMath::Clamp(R.Pitch + Axis.Y, MinLookPitch, MaxLookPitch);
	R.Yaw += Axis.X;
	SetControlRotation(R);
}

void ADFRunPlayerController::Input_JumpStart()
{
	if (ADFPlayerCharacter* const Hero = GetHeroPawn())
	{
		Hero->Jump();
	}
}

void ADFRunPlayerController::Input_JumpEnd()
{
	if (ADFPlayerCharacter* const Hero = GetHeroPawn())
	{
		Hero->StopJumping();
	}
}

void ADFRunPlayerController::Input_CameraZoom(const FInputActionValue& Value)
{
	if (ADFPlayerCharacter* const Hero = GetHeroPawn())
	{
		Hero->ApplyCameraZoomInput(Value.Get<float>());
	}
}

void ADFRunPlayerController::Input_AttackPressed()
{
	if (ADFPlayerCharacter* const Hero = GetHeroPawn())
	{
		Hero->HandlePrimaryAttackPressed();
	}
}

void ADFRunPlayerController::Input_AttackReleased()
{
	if (ADFPlayerCharacter* const Hero = GetHeroPawn())
	{
		Hero->HandlePrimaryAttackReleased();
	}
}

void ADFRunPlayerController::Input_SecondaryAttack()
{
	if (ADFPlayerCharacter* const Hero = GetHeroPawn())
	{
		Hero->HandleSecondaryAttackPressed();
	}
}

void ADFRunPlayerController::Input_Interact()
{
	if (ADFPlayerCharacter* const Hero = GetHeroPawn())
	{
		Hero->HandleInteractPressed();
	}
}

void ADFRunPlayerController::Input_SprintStart()
{
	if (ADFPlayerCharacter* const Hero = GetHeroPawn())
	{
		Hero->HandleSprintStart();
	}
}

void ADFRunPlayerController::Input_SprintEnd()
{
	if (ADFPlayerCharacter* const Hero = GetHeroPawn())
	{
		Hero->HandleSprintEnd();
	}
}

void ADFRunPlayerController::Input_Dodge()
{
	if (ADFPlayerCharacter* const Hero = GetHeroPawn())
	{
		Hero->HandleDodgePressed();
	}
}

void ADFRunPlayerController::Input_LockOn()
{
	if (ADFPlayerCharacter* const Hero = GetHeroPawn())
	{
		Hero->HandleLockOnToggle();
	}
}

void ADFRunPlayerController::Input_CycleLockOnLeft()
{
	if (ADFPlayerCharacter* const Hero = GetHeroPawn())
	{
		Hero->HandleCycleLockOnLeft();
	}
}

void ADFRunPlayerController::Input_CycleLockOnRight()
{
	if (ADFPlayerCharacter* const Hero = GetHeroPawn())
	{
		Hero->HandleCycleLockOnRight();
	}
}

void ADFRunPlayerController::Input_EquipmentWeaponToggle()
{
	if (ADFPlayerCharacter* const Hero = GetHeroPawn())
	{
		Hero->HandleEquipmentWeaponTogglePressed();
	}
}

void ADFRunPlayerController::SetupInputModeGameplay()
{
	EnsureGameplayInputReady();
}

void ADFRunPlayerController::SetupInputModeUIForWidget(UUserWidget* const Widget, const bool bPauseGame)
{
	if (bPauseGame)
	{
		UGameplayStatics::SetGamePaused(GetWorld(), true);
	}
	SetShowMouseCursor(true);
	FInputModeUIOnly M;
	M.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (Widget)
	{
		DFPrepareWidgetForUIModeFocus(Widget);
		M.SetWidgetToFocus(Widget->TakeWidget());
	}
	SetInputMode(M);
}

void ADFRunPlayerController::SetupInputModeUI()
{
	UUserWidget* const W = CharacterScreenInstance ? CharacterScreenInstance.Get() : PauseMenuInstance.Get();
	SetupInputModeUIForWidget(W, true);
}

void ADFRunPlayerController::ToggleInventory()
{
	if (CharacterScreenInstance)
	{
		CloseCharacterScreen();
		SetupInputModeGameplay();
		return;
	}
	if (!CharacterScreenClass)
	{
		return;
	}
	CharacterScreenInstance = CreateWidget<UUserWidget>(this, CharacterScreenClass);
	if (CharacterScreenInstance)
	{
		CharacterScreenInstance->AddToViewport(15);
		SetupInputModeUI();
	}
}

void ADFRunPlayerController::OnPause()
{
	if (PauseMenuInstance)
	{
		ClosePauseMenu();
		SetupInputModeGameplay();
		return;
	}
	if (!PauseMenuClass)
	{
		return;
	}
	PauseMenuInstance = CreateWidget<UUserWidget>(this, PauseMenuClass);
	if (PauseMenuInstance)
	{
		PauseMenuInstance->AddToViewport(20);
		SetupInputModeUIForWidget(PauseMenuInstance, true);
	}
}

void ADFRunPlayerController::CloseCharacterScreen()
{
	if (CharacterScreenInstance)
	{
		CharacterScreenInstance->RemoveFromParent();
		CharacterScreenInstance = nullptr;
	}
}

void ADFRunPlayerController::ClosePauseMenu()
{
	if (PauseMenuInstance)
	{
		PauseMenuInstance->RemoveFromParent();
		PauseMenuInstance = nullptr;
	}
	UGameplayStatics::SetGamePaused(GetWorld(), false);
}

void ADFRunPlayerController::Client_OpenVictoryScreen_Implementation(const FDFRunSummary Summary)
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.3f);
	if (VictoryScreenWidgetClass)
	{
		if (UDFVictoryScreenWidget* const W = CreateWidget<UDFVictoryScreenWidget>(this, VictoryScreenWidgetClass))
		{
			W->SetSummary(Summary);
			W->AddToViewport(20);
			SetupInputModeUIForWidget(W, true);
		}
	}
}

void ADFRunPlayerController::Client_OpenDefeatScreen_Implementation(
	const FDFRunSummary Summary, const FString& DefeatCause)
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
	if (DefeatScreenWidgetClass)
	{
		if (UDFDefeatScreenWidget* const W = CreateWidget<UDFDefeatScreenWidget>(this, DefeatScreenWidgetClass))
		{
			const FText CauseText = FText::FromString(DefeatCause);
			W->SetDefeatData(Summary, CauseText, FText::GetEmpty());
			W->AddToViewport(20);
			SetupInputModeUIForWidget(W, true);
		}
	}
}

void ADFRunPlayerController::PresentBetweenFloorFlow_Implementation()
{
	if (UWorld* const W = GetWorld())
	{
		if (UDFBetweenFloorSubsystem* const BF = W->GetSubsystem<UDFBetweenFloorSubsystem>())
		{
			BF->PresentStepForLocalPlayer(this);
		}
	}
}

void ADFRunPlayerController::Client_PresentBetweenFloorUI_Implementation()
{
	PresentBetweenFloorFlow();
}

void ADFRunPlayerController::Server_FinishBetweenFloorUI_Implementation()
{
	if (UWorld* const W = GetWorld())
	{
		if (UDFBetweenFloorSubsystem* const BF = W->GetSubsystem<UDFBetweenFloorSubsystem>())
		{
			BF->NotifyClientStepFinished(this);
		}
	}
}

bool ADFRunPlayerController::Server_FinishBetweenFloorUI_Validate()
{
	return true;
}

void ADFRunPlayerController::RequestReturnToNexus(const ERunNexusTravelReason Reason)
{
	if (IsLocalController())
	{
		Server_RequestReturnToNexus(Reason);
	}
}

void ADFRunPlayerController::Server_RequestReturnToNexus_Implementation(ERunNexusTravelReason const Reason)
{
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFWorldTransitionSubsystem* const T = GI->GetSubsystem<UDFWorldTransitionSubsystem>())
		{
			T->TravelToNexus(DFWorldTransition::NexusReasonToTravel(Reason));
		}
	}
}

bool ADFRunPlayerController::Server_RequestReturnToNexus_Validate(ERunNexusTravelReason const /*Reason*/)
{
	return true;
}

void ADFRunPlayerController::RequestPlayAgain() {}
