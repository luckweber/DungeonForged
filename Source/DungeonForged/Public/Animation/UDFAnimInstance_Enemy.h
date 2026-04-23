// Source/DungeonForged/Public/Animation/UDFAnimInstance_Enemy.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/UDFAnimInstance.h"
#include "UDFAnimInstance_Enemy.generated.h"

class UAnimMontage;

/**
 * Simplified anim instance for AI enemies. Extends UUDFAnimInstance so shared
 * root-motion notifies and CMC/ASC wiring still apply. Drive HitReactionDirection
 * from your hit reaction or damage system before calling SelectHitMontage.
 */
UCLASS(Blueprintable)
class DUNGEONFORGED_API UUDFAnimInstance_Enemy : public UUDFAnimInstance
{
	GENERATED_BODY()

public:
	/** World-space direction the last hit/crowd control came from (e.g. normalized attacker→self). */
	UPROPERTY(BlueprintReadWrite, Transient, Category = "DF|Hit")
	FVector HitReactionDirection = FVector::ZeroVector;

	/** Optional explicit hit origin for montage choice (e.g. damage causer). If zero, uses HitReactionDirection. */
	UPROPERTY(BlueprintReadWrite, Transient, Category = "DF|Hit")
	FVector HitFromWorldLocation = FVector::ZeroVector;

	UFUNCTION(BlueprintCallable, Category = "DF|Hit")
	UAnimMontage* SelectHitMontage(const FVector& WorldHitDirection) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Hit|Reactions")
	TObjectPtr<UAnimMontage> HitReact_Front;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Hit|Reactions")
	TObjectPtr<UAnimMontage> HitReact_Back;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Hit|Reactions")
	TObjectPtr<UAnimMontage> HitReact_Left;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Hit|Reactions")
	TObjectPtr<UAnimMontage> HitReact_Right;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Hit|Reactions")
	TObjectPtr<UAnimMontage> HitReact_Fallback;
};
