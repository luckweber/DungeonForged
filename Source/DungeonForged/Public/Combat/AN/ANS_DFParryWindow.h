// Source/DungeonForged/Public/Combat/AN/ANS_DFParryWindow.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_DFParryWindow.generated.h"

class UAnimSequenceBase;
class USkeletalMeshComponent;
struct FAnimNotifyEventReference;

/**
 * Parry window on an enemy windup.
 *
 * Drop on the few frames at the **start of the windup** where the attacker is committed but
 * vulnerable. Player strikes during this window trigger:
 *   • `Event.Combat.Parry.Triggered` on both attacker (victim) and player (instigator).
 *   • `ParryReactionGameplayEffect` (stun + bonus damage), authored as a GE asset on
 *     `UDFMeleeTraceComponent::ParryReactionGameplayEffect`.
 *
 * Side effects:
 *   • Adds loose tag `State.Combat.ParryWindow.Open` to the owner ASC.
 *   • Fires `Event.Combat.ParryWindow.Open` / `Event.Combat.ParryWindow.Close`.
 *
 * Pair with `UANS_DFEnemyTelegraph` so the player has a visible cue for WHEN to strike.
 */
UCLASS(Blueprintable, meta = (DisplayName = "DF Parry Window"))
class DUNGEONFORGED_API UANS_DFParryWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UANS_DFParryWindow();

	/** Send Event.Combat.ParryWindow.Open / Close on the ASC. */
	UPROPERTY(EditAnywhere, Category = "DF|Parry")
	bool bSendGameplayEvents = true;

	virtual FString GetNotifyName_Implementation() const override;
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
