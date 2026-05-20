// Source/DungeonForged/Public/Combat/AN/ANS_DFAbilityCancelWindow.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "ANS_DFAbilityCancelWindow.generated.h"

class UAnimSequenceBase;
class USkeletalMeshComponent;
struct FAnimNotifyEventReference;

/**
 * Generic ability-to-ability cancel window (see docs/improvements/03_Combat.md §6).
 *
 * During this notify, abilities whose tags match @c AllowedCancelTags may activate even if
 * @c ActivationBlockedTags would normally prevent it (@c UDFGameplayAbility::CanActivateAbility).
 */
UCLASS(Blueprintable, meta = (DisplayName = "DF Ability Cancel Window"))
class DUNGEONFORGED_API UANS_DFAbilityCancelWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UANS_DFAbilityCancelWindow();

	/** Ability tags that may cancel into this window (e.g. Ability.Mage.ArcaneBarrage). Empty = none. */
	UPROPERTY(EditAnywhere, Category = "DF|Cancel")
	FGameplayTagContainer AllowedCancelTags;

	virtual FString GetNotifyName_Implementation() const override;
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
