// Source/DungeonForged/Public/UI/ClassSelection/UDFClassSelectionSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Data/DFDataTableStructs.h"
#include "Subsystems/WorldSubsystem.h"
#include "UDFClassSelectionSubsystem.generated.h"

class ADFPlayerCharacter;
class UDFClassPreviewRotatorComponent;
class UDFSaveGame;
class UDataTable;
class USpringArmComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

/**
 * Drives 3D preview, slow-mo, and GameplayTag state for the class selection flow.
 * @see UDFClassSelectionWidget (WBP_ClassSelection parent C++ class).
 */
UCLASS()
class DUNGEONFORGED_API UDFClassSelectionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	FName GetSelectedClass() const { return SelectedClass; }
	void SetSelectedClass(const FName InClass) { SelectedClass = InClass; }

	/** Defaults to UDFRunManager::ClassDataTable on the GameInstance if unset. */
	UPROPERTY()
	TObjectPtr<UDataTable> ClassTable = nullptr;

	UPROPERTY()
	TObjectPtr<UDFSaveGame> SaveRef = nullptr;

	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection")
	TSubclassOf<ADFPlayerCharacter> PreviewPawnClass;

	/** If no actor in the world has the tag "ClassSelectionPreview", this transform is used. */
	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|Preview")
	FVector PreviewSpawnLocation = FVector(0.f, 5000.f, 120.f);

	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|Preview")
	FRotator PreviewSpawnRotation = FRotator(0.f, 180.f, 0.f);

	/** Off-screen; designer can place a "ClassSelectionPreview" target actor in the Nexus map. */
	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|Preview", meta = (ClampMin = "0.0"))
	float PreviewTimeDilation = 0.3f;

	/** If null, a transient RT is created. */
	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|Preview")
	TObjectPtr<UTextureRenderTarget2D> PreviewRenderTarget = nullptr;

	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|Preview")
	int32 RenderTargetWidth = 1024;

	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|Preview")
	int32 RenderTargetHeight = 1024;

	/** Widget class: parent UDFClassSelectionWidget; if null, uses CDO class. */
	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|UI")
	TSubclassOf<class UUserWidget> ClassSelectionWidgetClass;

	UFUNCTION(BlueprintCallable, Category = "DF|ClassSelection")
	void OpenClassSelection();

	/**
	 * Restores time + tags + preview. If @a bConfirm, starts the run (server) via Server RPC from local PC.
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|ClassSelection")
	void CloseClassSelection(bool bConfirm);

	UFUNCTION(BlueprintCallable, Category = "DF|ClassSelection|Preview")
	void UpdatePreviewForClass(FName ClassName);

	const FDFClassTableRow* GetClassData(FName ClassName) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DF|ClassSelection|Meta")
	bool IsClassUnlocked(FName ClassName) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DF|ClassSelection|Meta")
	FText GetUnlockConditionText(FName ClassName) const;

	UFUNCTION(BlueprintCallable, Category = "DF|ClassSelection|Preview")
	void RotatePreview(float YawDelta);

	UFUNCTION(BlueprintCallable, Category = "DF|ClassSelection|Preview")
	void ZoomPreview(float Delta);

	UFUNCTION(BlueprintCallable, Category = "DF|ClassSelection|Preview")
	ADFPlayerCharacter* GetPreviewPawn() const { return SpawnedPreviewPawn; }

	UFUNCTION(BlueprintCallable, Category = "DF|ClassSelection|Preview")
	UTextureRenderTarget2D* GetRenderTarget() const { return ActiveRenderTarget; }

	/** Merged CDO / @c UDFRunManager::ClassDataTable. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DF|ClassSelection|Data")
	UDataTable* GetClassTable() const;

	UDFClassPreviewRotatorComponent* GetRotator() const { return Rotator; }

	/** Normalized 0-1 for stat bars: Strength, Int, Agility, Defense (avg armor+MR), MaxHealth. */
	UFUNCTION(BlueprintCallable, Category = "DF|ClassSelection|UI")
	void GetStatBarScalesForClass(
		FName ClassName, float& OutStr, float& OutInt, float& OutAgi, float& OutDef, float& OutMaxHp) const;

protected:
	void ApplyUiTag(APlayerController* PC, bool bAdd);
	void SpawnPreviewPawn();
	void DestroyPreviewPawn();
	void EnsureSaveLoaded();
	void EnsureClassTable();
	void PlayClassIdleAnimation(const FDFClassTableRow& Row) const;
	void SpawnClassChangeVfx(const FDFClassTableRow& Row) const;
	void ApplyClassTintToPreview(const FLinearColor& Tint) const;

	UPROPERTY()
	FName SelectedClass = NAME_None;

	UPROPERTY()
	TObjectPtr<ADFPlayerCharacter> SpawnedPreviewPawn = nullptr;

	UPROPERTY()
	TObjectPtr<UDFClassPreviewRotatorComponent> Rotator = nullptr;

	UPROPERTY()
	TObjectPtr<USpringArmComponent> PreviewSpringArm = nullptr;

	UPROPERTY()
	TObjectPtr<USceneCaptureComponent2D> PreviewSceneCapture = nullptr;

	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> ActiveRenderTarget = nullptr;

	UPROPERTY()
	TObjectPtr<class UUserWidget> ClassSelectionWidgetInstance = nullptr;

	float PreviousGlobalTimeDilation = 1.f;
};
