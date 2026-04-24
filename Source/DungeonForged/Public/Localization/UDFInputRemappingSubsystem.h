// Source/DungeonForged/Public/Localization/UDFInputRemappingSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "UDFInputRemappingSubsystem.generated.h"

class UInputMappingContext;
class ULocalPlayer;
class UEnhancedInputLocalPlayerSubsystem;

UCLASS()
class DUNGEONFORGED_API UDFInputRemappingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** IMC to register for mappable key rows (set on subsystem default CDO or at runtime). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Input")
	TObjectPtr<UInputMappingContext> CurrentIMC = nullptr;

	/**
	 * Runtime cache of the last remapped key per row (FMapPlayerKeyArgs::MappingName).
	 * @see FEnhancedActionKeyMapping / Input Action "mapping name" in Enhanced Input mappable options.
	 */
	TMap<FName, FKey> RemappedKeys;

	UFUNCTION(BlueprintCallable, Category = "DF|Input")
	void RemapKey(ULocalPlayer* LocalPlayer, FName InputActionOrMappingName, FKey NewKey);

	/** Restores the active Enhanced Input profile defaults and clears the meta-save binding list. */
	UFUNCTION(BlueprintCallable, Category = "DF|Input")
	void ResetToDefaults(ULocalPlayer* LocalPlayer);

	UFUNCTION(BlueprintCallable, Category = "DF|Input")
	void SaveRemapping();

	UFUNCTION(BlueprintCallable, Category = "DF|Input")
	void LoadRemapping();

	/** Call after a local player exists (e.g. PC BeginPlay) so mappable key rows are registered. */
	UFUNCTION(BlueprintCallable, Category = "DF|Input")
	bool RegisterInputMappingContextForLocalPlayer(ULocalPlayer* LocalPlayer, UInputMappingContext* IMC = nullptr);

protected:
	void ApplyAllSavedToUserSettings(ULocalPlayer* LocalPlayer);

	static UEnhancedInputLocalPlayerSubsystem* GetEISS(ULocalPlayer* LocalPlayer);
};
