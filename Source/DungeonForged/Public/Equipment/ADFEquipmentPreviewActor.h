// Source/DungeonForged/Public/Equipment/ADFEquipmentPreviewActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ADFEquipmentPreviewActor.generated.h"

class ACharacter;
class UDFPreviewCaptureComponent;
class USkeletalMeshComponent;
class UTextureRenderTarget2D;
class ADFPlayerCharacter;

/**
 * Client-only paper doll: spawn when the character/equipment UI opens, destroy when it closes.
 * Mirrors ADFPlayerCharacter modular meshes for leader-pose preview; not replicated.
 */
UCLASS(BlueprintType, Blueprintable)
class DUNGEONFORGED_API ADFEquipmentPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	ADFEquipmentPreviewActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Equipment|Preview")
	TObjectPtr<USkeletalMeshComponent> PreviewMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Equipment|Preview")
	TObjectPtr<USkeletalMeshComponent> Mesh_Helmet = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Equipment|Preview")
	TObjectPtr<USkeletalMeshComponent> Mesh_Chest = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Equipment|Preview")
	TObjectPtr<USkeletalMeshComponent> Mesh_Legs = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Equipment|Preview")
	TObjectPtr<USkeletalMeshComponent> Mesh_Boots = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Equipment|Preview")
	TObjectPtr<USkeletalMeshComponent> Mesh_Gloves = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Equipment|Preview")
	TObjectPtr<USkeletalMeshComponent> Mesh_Weapon = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Equipment|Preview")
	TObjectPtr<USkeletalMeshComponent> Mesh_OffHand = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Equipment|Preview")
	TObjectPtr<UDFPreviewCaptureComponent> PreviewCapture = nullptr;

	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|Preview")
	void InitializePreview(UTextureRenderTarget2D* RenderTarget);

	/** Copies skeletal assets + anim classes + leader pose from a DF hero (modular slots). Generic characters: body mesh only. */
	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|Preview")
	void SyncMeshFromCharacter(ACharacter* SourceCharacter);

	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|Preview")
	void SetPreviewActive(bool bActive);

	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|Preview")
	void AddOrbitDeltaYaw(float DeltaDegrees);

protected:
	virtual void BeginPlay() override;

	static void CopySkelVisual(
		USkeletalMeshComponent* Dest,
		USkeletalMeshComponent* Src,
		USkeletalMeshComponent* LeaderForSlaves);

	void ApplyModularDefaults();
	void SyncFromDFPlayer(ADFPlayerCharacter* Source);
};
