// Source/DungeonForged/Public/Characters/DFAnimInstance.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "DFAnimInstance.generated.h"

class ACharacter;
class UAbilitySystemComponent;
class UDFCharacterMovementComponent;
struct FGameplayTag;

UCLASS()
class DUNGEONFORGED_API UUDFAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** Uses ASC tag query. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DF|Locomotion")
	bool HasTag(const FGameplayTag& Tag) const;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	TObjectPtr<ACharacter> OwningCharacter;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	TObjectPtr<UDFCharacterMovementComponent> DFCharacterMovement;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	TObjectPtr<UAbilitySystemComponent> OwningAbilitySystem;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	float Speed = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	float Direction = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bIsInAir = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bIsDodging = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bIsInCombat = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bIsLockedOn = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	FVector Velocity = FVector::ZeroVector;
};
