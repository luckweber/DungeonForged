// Source/DungeonForged/Public/Network/UDFNetworkValidator.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UDFNetworkValidator.generated.h"

class AActor;
class UAbilitySystemComponent;
class UWorld;

/**
 * Server-oriented validation helpers (anti-cheat policy surface). Call only with world authority;
 * Blueprint-exposed checks return false on clients so UI can branch safely.
 */
UCLASS()
class DUNGEONFORGED_API UDFNetworkValidator : public UObject
{
	GENERATED_BODY()

public:
	/** True if the world is playing in a host / dedicated / standalone role (not a pure network client). */
	UFUNCTION(BlueprintCallable, Category = "DF|Net|Validation", meta = (WorldContext = "WorldContextObject"))
	static bool HasWorldAuthority(UObject const* WorldContextObject);

	/**
	 * Gameplay abilities: the server must activate and apply effects. Clients use prediction only
	 * through `UAbilitySystemComponent` with valid `FPredictionKey` flow; never trust raw Client→Server
	 * damage payloads that skip GAS.
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|Net|Validation")
	static bool ValidateAbilityAuthority(UAbilitySystemComponent* ASC);

	/** Gold / currency: mutations only on `HasAuthority()` game code paths (PlayerState, RunManager). */
	UFUNCTION(BlueprintCallable, Category = "DF|Net|Validation")
	static bool ValidateEconomyAuthority(UObject const* SourceObject);

	/**
	 * Optional: compare pawn location to nav mesh. If delta to projected point exceeds `MaxDistance`,
	 * caller may snap (anti–speed-hack heuristic; tune for your CMC net mode).
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|Net|Validation")
	static bool IsPawnNearNavMesh(UWorld* World, AActor* Pawn, float MaxDistance);
};
