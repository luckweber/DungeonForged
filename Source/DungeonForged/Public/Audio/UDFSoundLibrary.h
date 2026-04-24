// Source/DungeonForged/Public/Audio/UDFSoundLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "UDFSoundLibrary.generated.h"

class USoundBase;

/**
 * Global tagged SFX table. Design-time only: add rows in the DataAsset.
 * Suggested content tags (also listed in the asset comment):
 * SFX.Ability.Fireball.Cast, SFX.Ability.Fireball.Impact, SFX.Ability.FrostBolt.Cast,
 * SFX.Ability.Melee.Swing, SFX.Ability.Melee.Hit, SFX.Ability.Dodge,
 * SFX.UI.Purchase, SFX.UI.LevelUp, SFX.Enemy.Death.Normal, SFX.Enemy.Death.Boss,
 * SFX.Dungeon.DoorOpen, SFX.Dungeon.ChestOpen, SFX.Dungeon.TrapTrigger
 */
UCLASS(Blueprintable)
class DUNGEONFORGED_API UDFSoundLibrary : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Map gameplay tags to SFX. Bind MetaSound (UMetaSoundSource) or SoundCue. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Audio|Tagged SFX")
	TMap<FGameplayTag, TObjectPtr<USoundBase>> TaggedSounds;

	/** O(1) lookup; null if not found. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DF|Audio|Tagged SFX")
	USoundBase* GetSoundForTag(const FGameplayTag& Tag) const;

	/** Uses Library when non-null; otherwise returns nullptr. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DF|Audio|Tagged SFX")
	static USoundBase* GetSound(const UDFSoundLibrary* Library, const FGameplayTag& Tag);
};
