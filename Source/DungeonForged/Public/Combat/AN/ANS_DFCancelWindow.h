// Source/DungeonForged/Public/Combat/AN/ANS_DFCancelWindow.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_DFCancelWindow.generated.h"

class UAnimSequenceBase;
class USkeletalMeshComponent;
struct FAnimNotifyEventReference;

/**
 * Marks the cancel window of a melee montage.
 *
 * Drop on a montage section where the attacker should be allowed to cancel into:
 *   - Heavy attack: gated by `UDFAbility_Warrior_HeavyAttack::CanActivateAbility` checking this tag.
 *   - Future: ability X / Y can require this tag too.
 *
 * Dodge is intentionally NOT gated by this — it's an emergency / defensive cancel that always fires.
 *
 * Side effects (Begin → End):
 *   • Adds loose tag `State.Combat.CancelWindow.Open` to the owner ASC.
 *   • Fires `Event.Combat.CancelWindow.Open` / `Event.Combat.CancelWindow.Close` so abilities listen.
 *
 * Designer placement: usually the LATE swing → recovery window (e.g. last 30% of montage).
 */
UCLASS(Blueprintable, meta = (DisplayName = "DF Cancel Window"))
class DUNGEONFORGED_API UANS_DFCancelWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UANS_DFCancelWindow();

	/** Send Event.Combat.CancelWindow.Open / Close on the ASC. */
	UPROPERTY(EditAnywhere, Category = "DF|Cancel")
	bool bSendGameplayEvents = true;

	/** Opens the combo chain window (same as @c AN_ComboWindowOpen). Enable when montages only use this notify. */
	UPROPERTY(EditAnywhere, Category = "DF|Cancel|Combo")
	bool bOpenComboWindowOnBegin = true;

	/** At window end, chain if the player buffered an attack during the window. */
	UPROPERTY(EditAnywhere, Category = "DF|Cancel|Combo")
	bool bAdvanceComboOnEndIfBuffered = true;

	virtual FString GetNotifyName_Implementation() const override;
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
