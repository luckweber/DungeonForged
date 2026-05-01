// Source/DungeonForged/Public/UI/ClassSelection/UDFClassSelectionSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Data/DFDataTableStructs.h"
#include "GameModes/MainMenu/DFMainMenuTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "UDFClassSelectionSubsystem.generated.h"

class UDFClassPreviewRotatorComponent;
class UDFSaveGame;
class UDataTable;
class USpringArmComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

/**
 * Drives 3D preview, slow-mo, and GameplayTag state for the class selection flow.
 * @see UDFClassSelectionWidget (WBP_ClassSelection parent C++ class).
 *
 * @note Edit -> Project Settings -> @c Dungeon Forged | Class Selection
 * (@c UDFClassSelectionDeveloperSettings) sobrescreve valores do
 * @c DefaultGame.ini após @c LoadConfig, incluindo DT de classes em Project Settings. Propriedades Config = Game também
 * podem ser ajustadas diretamente no .ini.
 * Exemplo:
 * @code{.ini}
 * [/Script/DungeonForged.DFClassSelectionSubsystem]
 * ClassSelectionWidgetClass=/Game/DungeonForged/UI/ClassSelection/WBP_ClassSelection.WBP_ClassSelection_C
 * PreviewPawnClass=/Game/.../BP_ClassPreview.BP_ClassPreview_C  (@c ADFClassPreviewCharacter recomendado)
 * @endcode
 */
UCLASS(Config = Game, DefaultConfig)
class DUNGEONFORGED_API UDFClassSelectionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	FName GetSelectedClass() const { return SelectedClass; }
	void SetSelectedClass(const FName InClass) { SelectedClass = InClass; }

	/** Defaults to UDFRunManager::ClassDataTable on the GameInstance if unset. */
	UPROPERTY()
	TObjectPtr<UDataTable> ClassTable = nullptr;

	UPROPERTY()
	TObjectPtr<UDFSaveGame> SaveRef = nullptr;

	/** Preferir BP derivado de @c ADFClassPreviewCharacter (leve); herói completo também é aceite. */
	UPROPERTY(Config, EditAnywhere, Category = "DF|ClassSelection")
	TSubclassOf<ACharacter> PreviewPawnClass;

	/**
	 * Soft path resolvido em @c EnsureWidgetClassResolved se @c ClassSelectionWidgetClass nao for atribuido.
	 * Default: @c WBP_ClassSelection seguindo @c MainMenu_Setup.md / @c ClassSelection_Setup.md.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "DF|ClassSelection|UI")
	FSoftClassPath ClassSelectionWidgetSoftPath = FSoftClassPath(
		TEXT("/Game/DungeonForged/UI/ClassSelection/WBP_ClassSelection.WBP_ClassSelection_C"));

	/** If no actor in the world has the tag "ClassSelectionPreview", this transform is used. */
	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|Preview")
	FVector PreviewSpawnLocation = FVector(0.f, 5000.f, 120.f);

	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|Preview")
	FRotator PreviewSpawnRotation = FRotator(0.f, 180.f, 0.f);

	/** Off-screen; designer can place a "ClassSelectionPreview" target actor in the Nexus map. */
	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|Preview", meta = (ClampMin = "0.0"))
	float PreviewTimeDilation = 0.3f;

	/**
	 * Luz pontual no pawn de preview (soma com Directional/Sky do nível). Desligue no L_MainMenu iluminado ou baixe @a PreviewFillLightLumens.
	 */
	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|Preview|Lighting")
	bool bPreviewUseFillLight = true;

	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|Preview|Lighting", meta = (EditCondition = "bPreviewUseFillLight"))
	float PreviewFillLightLumens = 2000.f;

	/**
	 * Se verdadeiro: SpringArm + SceneCapture → Render Target no UMG (centro do WBP).
	 * Se falso: pawn iluminado no mundo — típico Main Menu com painéis laterais e centro transparente / sem Image RT.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "DF|ClassSelection|Preview")
	bool bPreviewUsesSceneCapture = false;

	/** Distância à frente da câmara em modo mundo direto (ignorado se existir actor tag ClassSelectionPreview). */
	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|Preview|World", meta = (EditCondition = "!bPreviewUsesSceneCapture", EditConditionHides))
	float WorldPreviewDistanceFromCamera = 380.f;

	/** If null, a transient RT is created (somente quando @a bPreviewUsesSceneCapture). */
	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|Preview", meta = (EditCondition = "bPreviewUsesSceneCapture", EditConditionHides))
	TObjectPtr<UTextureRenderTarget2D> PreviewRenderTarget = nullptr;

	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|Preview", meta = (EditCondition = "bPreviewUsesSceneCapture", EditConditionHides))
	int32 RenderTargetWidth = 1024;

	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|Preview", meta = (EditCondition = "bPreviewUsesSceneCapture", EditConditionHides))
	int32 RenderTargetHeight = 1024;

	/** Widget class: parent @c UDFClassSelectionWidget; resolvido via @c ClassSelectionWidgetSoftPath se vazio. */
	UPROPERTY(Config, EditAnywhere, Category = "DF|ClassSelection|UI")
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
	ACharacter* GetPreviewPawn() const { return SpawnedPreviewPawn; }

	UFUNCTION(BlueprintCallable, Category = "DF|ClassSelection|Preview")
	UTextureRenderTarget2D* GetRenderTarget() const { return ActiveRenderTarget; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DF|ClassSelection|Preview")
	bool IsPreviewUsingSceneCapture() const { return bPreviewUsesSceneCapture; }

	/** When opening class selection from the main menu, set where to travel on confirm. */
	UFUNCTION(BlueprintCallable, Category = "DF|ClassSelection|MainMenu")
	void SetMainMenuClassPickDestination(EDFMainMenuClassPickDestination InDestination)
		{ MainMenuClassDestination = InDestination; }

	EDFMainMenuClassPickDestination GetMainMenuClassPickDestination() const
		{ return MainMenuClassDestination; }

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
	/** Primeira classe desbloqueada na DT → mesh/tint/animação no preview (antes só havia mesh do BP ao clicar). */
	void EnsureInitialPreviewClass();
	void PositionPreviewForDirectWorldView(APlayerController* PC);
	void EnsureSaveLoaded();
	void EnsureClassTable();
	/** Resolve @c ClassSelectionWidgetClass via SoftPath se ainda nao houver classe atribuida. */
	void EnsureWidgetClassResolved();
	void PlayClassIdleAnimation(const FDFClassTableRow& Row) const;
	void SpawnClassChangeVfx(const FDFClassTableRow& Row) const;
	void ApplyClassTintToPreview(const FLinearColor& Tint) const;

	UPROPERTY()
	FName SelectedClass = NAME_None;

	UPROPERTY()
	TObjectPtr<ACharacter> SpawnedPreviewPawn = nullptr;

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

	EDFMainMenuClassPickDestination MainMenuClassDestination = EDFMainMenuClassPickDestination::None;

	/** Zoom em modo mundo direto (distância ao longo do forward da câmara). */
	float ActiveWorldPreviewDistance = 380.f;

	/** Spawn usou actor tag ClassSelectionPreview — não reposicionar à frente da câmara. */
	bool bPreviewSpawnUsedTaggedAnchor = false;

	/** Main menu escondeu Main/SaveSlot para o cenário 3D aparecer sob WBP_ClassSelection. */
	bool bMainMenuLayersSuppressedForWorldPreview = false;
};
