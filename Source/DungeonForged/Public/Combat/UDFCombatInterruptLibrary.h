// Source/DungeonForged/Public/Combat/UDFCombatInterruptLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UDFCombatInterruptLibrary.generated.h"

class AActor;
class UGameplayEffect;

/** Boss cast interrupt helpers (Patch 8). */
UCLASS()
class DUNGEONFORGED_API UDFCombatInterruptLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * If @a Target has @c State.Combat.Casting.Interruptible, cancel active abilities/montage,
	 * apply optional stun GE, and broadcast @c Event.Combat.Boss.Interrupted.
	 * @return true when an interrupt was applied.
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|Combat|Interrupt", meta = (WorldContext = "WorldContextObject"))
	static bool TryInterruptBossCast(
		UObject* WorldContextObject,
		AActor* Target,
		AActor* Instigator,
		TSubclassOf<UGameplayEffect> BonusStunEffect = nullptr,
		float BonusStunDuration = 2.f);
};
