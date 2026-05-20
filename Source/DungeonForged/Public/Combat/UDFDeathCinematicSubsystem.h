// Source/DungeonForged/Public/Combat/UDFDeathCinematicSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Combat/UDFDeathCinematicTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "UDFDeathCinematicSubsystem.generated.h"

class ADFPlayerCharacter;

/** Central cinematic dispatch for player death and enemy kill moments (Silent Death report §5.2). */
UCLASS()
class DUNGEONFORGED_API UDFDeathCinematicSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DF|Death|Cinematic")
	void PlayPlayerDeathCinematic(ADFPlayerCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "DF|Death|Cinematic")
	void PlayEnemyKillCinematic(const FDFDeathCinematicContext& Context);

	UFUNCTION(BlueprintCallable, Category = "DF|Death|Cinematic")
	void PlayLastEnemyKillCinematic(AActor* Killer, AActor* LastVictim);

	/** Resolves defeat-screen attribution string from lethal context on the player. */
	UFUNCTION(BlueprintCallable, Category = "DF|Death|Cinematic")
	static FString ResolveLethalCauseString(
		AActor* Victim,
		AActor* Instigator,
		AActor* Causer,
		const FGameplayTagContainer& Tags);

	/** Resets active hit-stop / time dilation when skipping defeat UI. */
	UFUNCTION(BlueprintCallable, Category = "DF|Death|Cinematic")
	void ClearDeathTimeEffects();
};
