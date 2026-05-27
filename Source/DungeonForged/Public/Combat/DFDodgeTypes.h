// Source/DungeonForged/Public/Combat/DFDodgeTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DFDodgeTypes.generated.h"

class UAnimMontage;
class UAnimSequenceBase;

/** True if montage (or its sequences) uses root motion. Implemented in DFDodgeTypes.cpp. */
DUNGEONFORGED_API bool DFMontageHasRootMotion(const UAnimMontage* Montage);

/** First anim sequence referenced by the montage's primary slot track. */
DUNGEONFORGED_API UAnimSequenceBase* DFGetPrimaryMontageSequence(const UAnimMontage* Montage);

/** First slot track name on the montage (matches AnimBP slot node name). */
inline FName DFGetMontagePrimarySlotName(const UAnimMontage* Montage)
{
	if (Montage && Montage->SlotAnimTracks.Num() > 0)
	{
		return Montage->SlotAnimTracks[0].SlotName;
	}
	return NAME_None;
}

/** Octant order matches atan2(Right, Forward) snapped to 45° sectors. */
UENUM(BlueprintType)
enum class EDFDodgeDirection : uint8
{
	Forward UMETA(DisplayName = "Forward"),
	ForwardRight UMETA(DisplayName = "Forward Right"),
	Right UMETA(DisplayName = "Right"),
	BackwardRight UMETA(DisplayName = "Backward Right"),
	Backward UMETA(DisplayName = "Backward"),
	BackwardLeft UMETA(DisplayName = "Backward Left"),
	Left UMETA(DisplayName = "Left"),
	ForwardLeft UMETA(DisplayName = "Forward Left")
};

/** Per-stance dodge montages (8-way). Null entries use fallback chain in UDFAbility_Dodge. */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFDodgeAnimSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Dodge|AnimSet")
	TObjectPtr<UAnimMontage> Forward = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Dodge|AnimSet")
	TObjectPtr<UAnimMontage> ForwardRight = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Dodge|AnimSet")
	TObjectPtr<UAnimMontage> Right = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Dodge|AnimSet")
	TObjectPtr<UAnimMontage> BackwardRight = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Dodge|AnimSet")
	TObjectPtr<UAnimMontage> Backward = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Dodge|AnimSet")
	TObjectPtr<UAnimMontage> BackwardLeft = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Dodge|AnimSet")
	TObjectPtr<UAnimMontage> Left = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Dodge|AnimSet")
	TObjectPtr<UAnimMontage> ForwardLeft = nullptr;

	UAnimMontage* Resolve(const EDFDodgeDirection Dir) const
	{
		switch (Dir)
		{
		case EDFDodgeDirection::Forward:
			return Forward;
		case EDFDodgeDirection::ForwardRight:
			return ForwardRight;
		case EDFDodgeDirection::Right:
			return Right;
		case EDFDodgeDirection::BackwardRight:
			return BackwardRight;
		case EDFDodgeDirection::Backward:
			return Backward;
		case EDFDodgeDirection::BackwardLeft:
			return BackwardLeft;
		case EDFDodgeDirection::Left:
			return Left;
		case EDFDodgeDirection::ForwardLeft:
			return ForwardLeft;
		default:
			return Backward;
		}
	}

	bool IsValid() const
	{
		return Forward || ForwardRight || Right || BackwardRight || Backward || BackwardLeft || Left || ForwardLeft;
	}
};

/** Unit direction in actor local space (X = forward, Y = right). */
inline FVector DFGetDodgeDirectionLocalVector(const EDFDodgeDirection Dir)
{
	switch (Dir)
	{
	case EDFDodgeDirection::Forward:
		return FVector::ForwardVector;
	case EDFDodgeDirection::ForwardRight:
		return FVector(1.f, 1.f, 0.f).GetSafeNormal();
	case EDFDodgeDirection::Right:
		return FVector::RightVector;
	case EDFDodgeDirection::BackwardRight:
		return FVector(-1.f, 1.f, 0.f).GetSafeNormal();
	case EDFDodgeDirection::Backward:
		return FVector::BackwardVector;
	case EDFDodgeDirection::BackwardLeft:
		return FVector(-1.f, -1.f, 0.f).GetSafeNormal();
	case EDFDodgeDirection::Left:
		return FVector::LeftVector;
	case EDFDodgeDirection::ForwardLeft:
		return FVector(1.f, -1.f, 0.f).GetSafeNormal();
	default:
		return FVector::BackwardVector;
	}
}

/**
 * Movement intent in actor local space (X = forward, Y = right).
 * Uses CMC last input; if too weak, falls back to velocity (same as UDFComboComponent).
 */
inline FVector DFResolveLocalMovementIntent(const ACharacter* Char, const UCharacterMovementComponent* CMC,
	const float DirectionalInputThreshold)
{
	if (!Char || !CMC)
	{
		return FVector::ZeroVector;
	}
	const float ThresholdSq = DirectionalInputThreshold * DirectionalInputThreshold;
	const FTransform& ActorXform = Char->GetActorTransform();
	FVector LocalInput = ActorXform.InverseTransformVectorNoScale(CMC->GetLastInputVector());
	if (LocalInput.SizeSquared2D() < ThresholdSq)
	{
		const FVector WorldVel = Char->GetVelocity();
		if (WorldVel.SizeSquared2D() >= ThresholdSq)
		{
			LocalInput = ActorXform.InverseTransformVectorNoScale(WorldVel);
		}
	}
	return LocalInput;
}

/** Snap local input (X forward, Y right) to nearest 45° octant. */
inline EDFDodgeDirection DFSnapLocalInputToDodgeDirection(const FVector& LocalInput)
{
	const float Angle = FMath::Atan2(LocalInput.Y, LocalInput.X);
	const int32 Octant = (FMath::RoundToInt(Angle / (PI / 4.f)) + 8) % 8;
	return static_cast<EDFDodgeDirection>(Octant);
}

/** Cardinal/diagonal fallbacks when a specific octant montage is missing. */
inline void DFGetDodgeDirectionResolveOrder(const EDFDodgeDirection Dir, TArray<EDFDodgeDirection>& Out)
{
	Out.Reset();
	Out.Add(Dir);
	switch (Dir)
	{
	case EDFDodgeDirection::ForwardLeft:
		Out.Add(EDFDodgeDirection::Forward);
		Out.Add(EDFDodgeDirection::Left);
		break;
	case EDFDodgeDirection::ForwardRight:
		Out.Add(EDFDodgeDirection::Forward);
		Out.Add(EDFDodgeDirection::Right);
		break;
	case EDFDodgeDirection::BackwardLeft:
		Out.Add(EDFDodgeDirection::Backward);
		Out.Add(EDFDodgeDirection::Left);
		break;
	case EDFDodgeDirection::BackwardRight:
		Out.Add(EDFDodgeDirection::Backward);
		Out.Add(EDFDodgeDirection::Right);
		break;
	default:
		break;
	}
	Out.Add(EDFDodgeDirection::Backward);
}
