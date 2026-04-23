// Source/DungeonForged/Private/Characters/DFAnimInstance.cpp
#include "Characters/DFAnimInstance.h"

#include "Characters/UDFCharacterMovementComponent.h"
#include "GAS/DFGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "KismetAnimationLibrary.h"

void UUDFAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	OwningCharacter = Cast<ACharacter>(GetOwningActor());
	DFCharacterMovement = OwningCharacter ? Cast<UDFCharacterMovementComponent>(OwningCharacter->GetCharacterMovement()) : nullptr;
	if (IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(OwningCharacter))
	{
		OwningAbilitySystem = IAS->GetAbilitySystemComponent();
	}
}

void UUDFAnimInstance::NativeUpdateAnimation(const float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (!OwningCharacter)
	{
		return;
	}
	if (!DFCharacterMovement)
	{
		DFCharacterMovement = Cast<UDFCharacterMovementComponent>(OwningCharacter->GetCharacterMovement());
	}
	if (DFCharacterMovement)
	{
		Velocity = DFCharacterMovement->Velocity;
	}
	else
	{
		Velocity = OwningCharacter->GetVelocity();
	}
	Speed = Velocity.Size();
	if (IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(OwningCharacter))
	{
		OwningAbilitySystem = IAS->GetAbilitySystemComponent();
	}
	if (OwningCharacter && Speed > 1.f)
	{
		const FRotator BaseRot(0.f, OwningCharacter->GetActorRotation().Yaw, 0.f);
		Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, BaseRot);
	}
	else
	{
		Direction = 0.f;
	}
	if (DFCharacterMovement)
	{
		bIsInAir = DFCharacterMovement->IsFalling();
		bIsSprinting = DFCharacterMovement->bIsSprinting;
		bIsDodging = DFCharacterMovement->bIsDodging;
	}
	else
	{
		bIsInAir = OwningCharacter->GetCharacterMovement() ? OwningCharacter->GetCharacterMovement()->IsFalling() : false;
		bIsSprinting = false;
		bIsDodging = false;
	}
	if (UAbilitySystemComponent* const ASC = OwningAbilitySystem.Get())
	{
		bIsDead = ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Dead);
		bIsInCombat = ASC->HasMatchingGameplayTag(FDFGameplayTags::State_InCombat);
		bIsLockedOn = ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Targeting);
	}
	else
	{
		bIsDead = false;
		bIsInCombat = false;
		bIsLockedOn = false;
	}
	(void)DeltaSeconds;
}

bool UUDFAnimInstance::HasTag(const FGameplayTag& Tag) const
{
	if (!Tag.IsValid() || !OwningAbilitySystem)
	{
		return false;
	}
	return OwningAbilitySystem->HasMatchingGameplayTag(Tag);
}
