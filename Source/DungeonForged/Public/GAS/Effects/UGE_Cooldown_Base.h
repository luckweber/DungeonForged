// Source/DungeonForged/Public/GAS/Effects/UGE_Cooldown_Base.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Cooldown_Base.generated.h"

/**
 * Cooldown window only: duration = SetByCaller Data.Cooldown.
 * Asset tags keep Ability.Cooldown; InheritableGameplayEffectTags also carry Ability.Cooldown plus
 * CooldownAssociatedAbilityTag so HUD queries (MatchAll) can match a specific slot.
 */
UCLASS()
class DUNGEONFORGED_API UGE_Cooldown_Base : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Cooldown_Base();

	/** Must match the ability tag used on the hotbar row / UDFAbilitySlotWidget (e.g. Ability.Warrior.MeleeSwing). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Cooldown", meta = (Categories = "Ability"))
	FGameplayTag CooldownAssociatedAbilityTag;

protected:
	virtual void ConfigureEffectCDO() override;
};
