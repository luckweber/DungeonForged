// Source/DungeonForged/Public/UI/Combat/UDFCombatTextSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "UI/Combat/DFCombatTextTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "UDFCombatTextSubsystem.generated.h"

class UDFCombatTextWidget;

/** Object pool of UDFCombatTextWidget for floating numbers (non-dedicated clients). */
UCLASS()
class DUNGEONFORGED_API UDFCombatTextSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void Initialize(FSubsystemCollectionBase& Collection) override;
	void Deinitialize() override;

	/** Formats value + type (damage, crit stars, +heal, etc.) and spawns. */
	UFUNCTION(BlueprintCallable, Category = "DF|CombatText", meta = (DisplayName = "Spawn Combat Text (Value)"))
	void SpawnText(FVector WorldLocation, float Value, ECombatTextType Type, float CustomDuration = -1.f);

	/** Pre-built string (MISS, DODGE, +240 XP, status names). */
	UFUNCTION(BlueprintCallable, Category = "DF|CombatText", meta = (DisplayName = "Spawn Combat Text (String)"))
	void SpawnTextString(FVector WorldLocation, const FString& Text, ECombatTextType Type, float CustomDuration = -1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|CombatText", meta = (DisplayName = "Widget Class (W_DF_CombatText)"))
	TSubclassOf<UDFCombatTextWidget> WidgetClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|CombatText", meta = (ClampMin = "1"))
	int32 PoolSize = 30;

	/** Called by widget when anim/timer ends. */
	void ReturnToPool(UDFCombatTextWidget* Widget);

protected:
	void EnsurePooledWidgets();

private:
	TArray<TObjectPtr<UDFCombatTextWidget>> Pooled;
	TArray<TObjectPtr<UDFCombatTextWidget>> InUse;
	bool bPoolBuilt = false;

	/** -1: type default. */
	float ResolveDuration(ECombatTextType Type, float Custom) const;
	static FString BuildStringForValue(float Value, ECombatTextType Type);
};
