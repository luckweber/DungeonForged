#include "Characters/ADFPlayerCharacter.h"

#include "Characters/ADFPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Camera/UDFCameraComponent.h"
#include "Camera/UDFLockOnComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GAS/UDFAttributeSet.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "GameplayTagContainer.h"
#include "Components/CapsuleComponent.h"

ADFPlayerCharacter::ADFPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = true;
	}

	CameraBoom = CreateDefaultSubobject<UDFCameraComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	LockOnComponent = CreateDefaultSubobject<UDFLockOnComponent>(TEXT("LockOnComponent"));

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
}

UAbilitySystemComponent* ADFPlayerCharacter::GetAbilitySystemComponent() const
{
	if (AbilitySystemComponent)
	{
		return AbilitySystemComponent;
	}
	if (const ADFPlayerState* PS = GetPlayerState<ADFPlayerState>())
	{
		return PS->AbilitySystemComponent;
	}
	return nullptr;
}

void ADFPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	AddDefaultMappingContext();
}

void ADFPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	// Pawn can restart (respawn) without a new BeginPlay — ensure local IMC is present.
	if (!bDefaultInputContextAdded)
	{
		AddDefaultMappingContext();
	}
}

void ADFPlayerCharacter::AddDefaultMappingContext()
{
	if (bDefaultInputContextAdded || !IMC_Default || !IsLocallyControlled())
	{
		return;
	}
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}
	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP)
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
	{
		Sub->AddMappingContext(IMC_Default, 0);
		bDefaultInputContextAdded = true;
	}
}

void ADFPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		return;
	}

	if (IA_Move)
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ADFPlayerCharacter::Input_Move);
	}
	if (IA_Look)
	{
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ADFPlayerCharacter::Input_Look);
	}
	if (IA_Jump)
	{
		EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &ADFPlayerCharacter::Input_JumpStart);
		EIC->BindAction(IA_Jump, ETriggerEvent::Completed, this, &ADFPlayerCharacter::Input_JumpEnd);
	}
	if (IA_CameraZoom)
	{
		EIC->BindAction(IA_CameraZoom, ETriggerEvent::Triggered, this, &ADFPlayerCharacter::Input_CameraZoom);
	}
	if (InputConfig)
	{
		RegisterAbilityInputFromConfig(EIC);
	}
	else
	{
		if (IA_Attack)
		{
			EIC->BindAction(IA_Attack, ETriggerEvent::Started, this, &ADFPlayerCharacter::Input_Attack);
		}
		if (IA_Ability1)
		{
			EIC->BindAction(IA_Ability1, ETriggerEvent::Started, this, &ADFPlayerCharacter::Input_Ability1);
		}
		if (IA_Ability2)
		{
			EIC->BindAction(IA_Ability2, ETriggerEvent::Started, this, &ADFPlayerCharacter::Input_Ability2);
		}
		if (IA_Ability3)
		{
			EIC->BindAction(IA_Ability3, ETriggerEvent::Started, this, &ADFPlayerCharacter::Input_Ability3);
		}
		if (IA_Ability4)
		{
			EIC->BindAction(IA_Ability4, ETriggerEvent::Started, this, &ADFPlayerCharacter::Input_Ability4);
		}
		if (IA_Interact)
		{
			EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &ADFPlayerCharacter::Input_Interact);
		}
	}
}

void ADFPlayerCharacter::RegisterAbilityInputFromConfig(UEnhancedInputComponent* EIC)
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
		int32 InputId = Row.GameplayInputId;
		if (InputId <= 0)
		{
			InputId = Index + 1;
		}
		const int32 CapturedId = InputId;
		EIC->BindActionValueLambda(Row.Action, ETriggerEvent::Started, [this, CapturedId](const FInputActionValue&)
		{
			if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
			{
				ASC->AbilityLocalInputPressed(CapturedId);
			}
		});
		EIC->BindActionValueLambda(Row.Action, ETriggerEvent::Completed, [this, CapturedId](const FInputActionValue&)
		{
			if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
			{
				ASC->AbilityLocalInputReleased(CapturedId);
			}
		});
	}
}

void ADFPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (HasAuthority())
	{
		InitializeGAS();
	}
}

void ADFPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitializeGAS();
}

void ADFPlayerCharacter::InitializeGAS()
{
	ADFPlayerState* PS = GetPlayerState<ADFPlayerState>();
	if (!PS)
	{
		AbilitySystemComponent = nullptr;
		AttributeSet = nullptr;
		return;
	}

	AbilitySystemComponent = PS->AbilitySystemComponent;
	AttributeSet = PS->AttributeSet;

	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		ASC->InitAbilityActorInfo(PS, this);
	}
}

void ADFPlayerCharacter::Input_Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Axis.IsNearlyZero())
	{
		return;
	}
	const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
	AddMovementInput(Forward, Axis.Y);
	AddMovementInput(Right, Axis.X);
}

void ADFPlayerCharacter::Input_Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->AddPitchInput(Axis.Y);
		PC->AddYawInput(Axis.X);
		ApplyLookPitchClamp();
	}
}

void ADFPlayerCharacter::ApplyLookPitchClamp()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FRotator R = PC->GetControlRotation();
		R.Pitch = FMath::Clamp(R.Pitch, MinLookPitch, MaxLookPitch);
		PC->SetControlRotation(R);
	}
}

void ADFPlayerCharacter::Input_JumpStart()
{
	Jump();
}

void ADFPlayerCharacter::Input_JumpEnd()
{
	StopJumping();
}

void ADFPlayerCharacter::Input_CameraZoom(const FInputActionValue& Value)
{
	UDFCameraComponent* const Cam = CameraBoom;
	if (!Cam)
	{
		return;
	}
	// Inverted wheel: zoom in = shorter arm; scales by ZoomSpeed on the camera
	Cam->OnZoomInput(-Value.Get<float>() * (CameraZoomStep / 50.f));
}

void ADFPlayerCharacter::Input_Attack()
{
	TryActivateByGameplayTagName(FName("Ability.Attack"));
}

void ADFPlayerCharacter::Input_Ability1()
{
	TryActivateAbilitySlot(1);
}

void ADFPlayerCharacter::Input_Ability2()
{
	TryActivateAbilitySlot(2);
}

void ADFPlayerCharacter::Input_Ability3()
{
	TryActivateAbilitySlot(3);
}

void ADFPlayerCharacter::Input_Ability4()
{
	TryActivateAbilitySlot(4);
}

void ADFPlayerCharacter::Input_Interact()
{
	// Hook for interaction traces / abilities — keep gameplay in C++ or forward to a subsystem
}

void ADFPlayerCharacter::TryActivateByGameplayTagName(const FName& TagName)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
		if (!Tag.IsValid())
		{
			return;
		}
		FGameplayTagContainer C;
		C.AddTag(Tag);
		ASC->TryActivateAbilitiesByTag(C, true);
	}
}

void ADFPlayerCharacter::TryActivateAbilitySlot(int32 Slot1Based)
{
	if (Slot1Based < 1 || Slot1Based > 4)
	{
		return;
	}
	const FName N(*FString::Printf(TEXT("Ability.Slot.%d"), Slot1Based));
	TryActivateByGameplayTagName(N);
}
