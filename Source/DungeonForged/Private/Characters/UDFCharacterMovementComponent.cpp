// Source/DungeonForged/Private/Characters/UDFCharacterMovementComponent.cpp
#include "Characters/UDFCharacterMovementComponent.h"

#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/RootMotionSource.h"
#include "GameplayEffect.h"
#include "TimerManager.h"

UDFCharacterMovementComponent::UDFCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	MaxWalkSpeed = WalkSpeed;
	MaxWalkSpeedCrouched = CrouchSpeed;
}

FNetworkPredictionData_Client* UDFCharacterMovementComponent::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		UDFCharacterMovementComponent* const MutableThis = const_cast<UDFCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_DF(*this);
	}
	return ClientPredictionData;
}

void UDFCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	const bool bSprintWanted = (Flags & static_cast<uint8>(FSavedMove_Character::FLAG_Custom_0)) != 0;
	SetSprinting(bSprintWanted, true);
}

void UDFCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	const EMovementMode NewMode = MovementMode;
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	OnDFMovementModeChanged.Broadcast(NewMode, PreviousMovementMode, PreviousCustomMode);
}

void UDFCharacterMovementComponent::TickComponent(
	float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshMaxWalkSpeed();
	if (bIsSprinting)
	{
		TickSprintStamina(DeltaTime);
	}
}

void UDFCharacterMovementComponent::SetSprintStaminaFromGameplayEffect(bool bFromEffect)
{
	bSprintStaminaFromGameplayEffect = bFromEffect;
}

void UDFCharacterMovementComponent::SetSprinting(const bool bSprint, const bool /*bFromNetworkPrediction*/)
{
	bIsSprinting = bSprint;
	RefreshMaxWalkSpeed();
}

void UDFCharacterMovementComponent::RefreshMaxWalkSpeed()
{
	if (IsCrouching())
	{
		MaxWalkSpeedCrouched = CrouchSpeed;
		return;
	}
	MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
}

void UDFCharacterMovementComponent::TickSprintStamina(const float DeltaTime)
{
	if (bSprintStaminaFromGameplayEffect || !bIsSprinting)
	{
		if (IAbilitySystemInterface* IAS = Cast<IAbilitySystemInterface>(CharacterOwner))
		{
			if (UAbilitySystemComponent* ASC = IAS->GetAbilitySystemComponent())
			{
				const FGameplayAttribute S = UDFAttributeSet::GetStaminaAttribute();
				if (S.IsValid() && ASC->GetNumericAttribute(S) <= 0.01f)
				{
					SetSprinting(false, false);
					ApplySprintExhaustionIfAny();
				}
			}
		}
		return;
	}

	IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(CharacterOwner);
	if (!IAS)
	{
		return;
	}
	UAbilitySystemComponent* const ASC = IAS->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}
	UDFAttributeSet* const Attrs = const_cast<UDFAttributeSet*>(ASC->GetSet<UDFAttributeSet>());
	if (!Attrs)
	{
		return;
	}
	const float Current = Attrs->GetStamina();
	const float Next = FMath::Max(0.f, Current - SprintStaminaDrain * DeltaTime);
	Attrs->SetStamina(Next);
	if (Next <= 0.01f)
	{
		SetSprinting(false, false);
		ApplySprintExhaustionIfAny();
	}
}

void UDFCharacterMovementComponent::ApplySprintExhaustionIfAny()
{
	if (!SprintExhaustionEffect)
	{
		return;
	}
	IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(CharacterOwner);
	if (!IAS)
	{
		return;
	}
	UAbilitySystemComponent* const ASC = IAS->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}
	if (!ASC->GetOwner() || !ASC->GetOwner()->HasAuthority())
	{
		return;
	}
	const FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(SprintExhaustionEffect, 1.f, Ctx);
	if (Spec.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

FVector UDFCharacterMovementComponent::GetDodgeDirection() const
{
	FVector D = GetLastInputVector();
	if (D.SizeSquared() > 0.01f)
	{
		return D.GetSafeNormal();
	}
	if (CharacterOwner)
	{
		return -CharacterOwner->GetActorForwardVector();
	}
	return FVector::ForwardVector;
}

void UDFCharacterMovementComponent::PerformDodge(const FVector& DirectionWorld)
{
	if (!CharacterOwner)
	{
		return;
	}
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	const float Now = W->GetTimeSeconds();
	if (TimeLastDodge >= 0.f && (Now - TimeLastDodge) < DodgeCooldown)
	{
		return;
	}
	TimeLastDodge = Now;
	FVector Dir = DirectionWorld;
	if (Dir.IsNearlyZero())
	{
		Dir = GetDodgeDirection();
	}
	else
	{
		Dir = Dir.GetSafeNormal();
	}

	IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(CharacterOwner);
	if (UAbilitySystemComponent* ASC = IAS ? IAS->GetAbilitySystemComponent() : nullptr)
	{
		ASC->AddLooseGameplayTag(FDFGameplayTags::State_Dodging);
		ASC->AddLooseGameplayTag(FDFGameplayTags::State_Invulnerable);
	}

	bIsDodging = true;

	if (IFrameDuration > 0.f)
	{
		W->GetTimerManager().SetTimer(
			TimerHandle_EndIFrame, this, &UDFCharacterMovementComponent::EndIFrameState, IFrameDuration, false);
	}
	if (DodgeDuration > 0.f)
	{
		W->GetTimerManager().SetTimer(
			TimerHandle_EndDodging, this, &UDFCharacterMovementComponent::EndDodgingState, DodgeDuration, false);
	}

	if (USceneComponent* const Comp = UpdatedComponent)
	{
		const FVector Start = Comp->GetComponentLocation();
		const FVector End = Start + Dir * DodgeDistance;
		const TSharedPtr<FRootMotionSource_MoveToForce> Source = MakeShared<FRootMotionSource_MoveToForce>();
		Source->InstanceName = FName(TEXT("DFDodge"));
		Source->Priority = 600;
		Source->Duration = DodgeDuration;
		Source->AccumulateMode = ERootMotionAccumulateMode::Override;
		Source->bInLocalSpace = false;
		Source->StartLocation = Start;
		Source->TargetLocation = End;
		Source->bRestrictSpeedToExpected = true;
		ApplyRootMotionSource(Source);
	}
}

void UDFCharacterMovementComponent::EndIFrameState()
{
	if (!CharacterOwner)
	{
		return;
	}
	if (IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(CharacterOwner))
	{
		if (UAbilitySystemComponent* const ASC = IAS->GetAbilitySystemComponent())
		{
			ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Invulnerable, 0);
		}
	}
}

void UDFCharacterMovementComponent::EndDodgingState()
{
	bIsDodging = false;
	if (!CharacterOwner)
	{
		return;
	}
	if (IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(CharacterOwner))
	{
		if (UAbilitySystemComponent* const ASC = IAS->GetAbilitySystemComponent())
		{
			ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Dodging, 0);
		}
	}
}

void FSavedMove_DF::Clear()
{
	FSavedMove_Character::Clear();
	bWantsSprint = false;
}

uint8 FSavedMove_DF::GetCompressedFlags() const
{
	uint8 F = FSavedMove_Character::GetCompressedFlags();
	if (bWantsSprint)
	{
		F |= static_cast<uint8>(FSavedMove_Character::FLAG_Custom_0);
	}
	return F;
}

void FSavedMove_DF::SetMoveFor(
	ACharacter* C,
	const float InDeltaTime,
	const FVector& NewAccel,
	FNetworkPredictionData_Client_Character& ClientData)
{
	FSavedMove_Character::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
	if (C)
	{
		if (const UDFCharacterMovementComponent* const DF = Cast<UDFCharacterMovementComponent>(C->GetCharacterMovement()))
		{
			bWantsSprint = DF->bIsSprinting;
		}
	}
}

void FSavedMove_DF::PrepMoveFor(ACharacter* C)
{
	FSavedMove_Character::PrepMoveFor(C);
	if (C)
	{
		if (UDFCharacterMovementComponent* const DF = Cast<UDFCharacterMovementComponent>(C->GetCharacterMovement()))
		{
			DF->SetSprinting(bWantsSprint, true);
		}
	}
}

bool FSavedMove_DF::CanCombineWith(
	const FSavedMovePtr& NewMove, ACharacter* InCharacter, const float MaxDelta) const
{
	if (!FSavedMove_Character::CanCombineWith(NewMove, InCharacter, MaxDelta))
	{
		return false;
	}
	if (const FSavedMove_DF* const B = static_cast<FSavedMove_DF*>(NewMove.Get()))
	{
		return bWantsSprint == B->bWantsSprint;
	}
	return true;
}

FNetworkPredictionData_DF::FNetworkPredictionData_DF(const UCharacterMovementComponent& ClientMovement)
	: FNetworkPredictionData_Client_Character(ClientMovement)
{
}

FSavedMovePtr FNetworkPredictionData_DF::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_DF());
}
