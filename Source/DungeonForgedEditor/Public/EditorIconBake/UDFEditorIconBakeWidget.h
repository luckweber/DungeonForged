// Copyright DungeonForged. EUW base C++; módulo Editor-only para não incluir EditorUtilityWidget no DLL de jogo.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"

#include "Editor/IconBake/UDFEditorIconBakeLibrary.h"
#include "UDFEditorIconBakeWidget.generated.h"
class AActor;

/** EUW base C++ para Icon Bake. Blueprintable para reparent em Editor Utility Widget no Content Browser. */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "DF Editor Icon Bake Widget"))
class DUNGEONFORGEDEDITOR_API UDFEditorIconBakeWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	/** Procura uma instância no mundo do Editor (lazy): se já houver cache válido, devolve esse actor. Define @a IconCreatorActorClass (ex. @c BP_IconCreator) nas Class Defaults. */
	UFUNCTION(BlueprintCallable, Category = "DF|Editor|IconBake",
		meta = (DevelopmentOnly, DisplayName = "DF Icon Bake · Obter / resolver Icon Creator Actor"))
	AActor* ResolveIconCreatorActor();

	/** Invalida cache (útil quando sais do nível ou após destroy do actor estúdio). */
	UFUNCTION(BlueprintCallable, Category = "DF|Editor|IconBake",
		meta = (DevelopmentOnly, DisplayName = "DF Icon Bake · Limpar cache Icon Creator Actor"))
	void ClearIconCreatorActorCache();

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Icon Studio")
	TSubclassOf<AActor> IconCreatorActorClass;

	/** Referência mantida pela função de resolução; Transient não persiste ao fechar Editor Utility. */
	UPROPERTY(BlueprintReadWrite, Category = "Icon Studio", meta = (DisplayName = "Cached Icon Creator Actor"))
	TObjectPtr<AActor> CachedIconCreatorActor;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Asset")
	EDFICAssetType AssetType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Asset",
		meta = (EditCondition = "AssetType == EDFICAssetType::StaticMesh", EditConditionHides))
	TObjectPtr<UStaticMesh> StaticMesh;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Asset",
		meta = (EditCondition = "AssetType == EDFICAssetType::SkeletalMesh", EditConditionHides))
	TObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Asset",
		meta = (EditCondition = "AssetType == EDFICAssetType::SkeletalMesh", EditConditionHides))
	TObjectPtr<UAnimationAsset> Anim;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Asset",
		meta = (EditCondition = "AssetType == EDFICAssetType::SkeletalMesh", EditConditionHides))
	float AnimPosition;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Asset",
		meta = (EditCondition = "AssetType == EDFICAssetType::Actor", EditConditionHides))
	TSubclassOf<AActor> Actor;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Asset")
	TObjectPtr<UMaterialInterface> OverlayMaterial;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Asset Position")
	bool Center = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Asset Position")
	FRotator AssetRotation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Asset Position", meta = (AllowPreserveRatio))
	FVector AssetScale = FVector(1, 1, 1);

	/** Camera field of view (in degrees). */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Camera", Interp, meta = (UIMin = "5.0", UIMax = "170", ClampMin = "0.001", ClampMax = "360.0"))
		float FieldOfView = 90;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Camera")
		float Distance = 300;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Camera")
		float ZoomStep = 5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Camera")
		FVector CameraLocation;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Camera")
		FRotator CameraRotation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lighting")
		bool CastShadows1 = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lighting")
		bool CastShadows2 = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lighting")
		bool CastShadows3 = true;



	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Save")
		FString IconName = "T_";

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Save")
		FString SavePath = "/Game/IconCreator";

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Save")
		UTextureRenderTarget2D* RenderTarget;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Save")
		FVector2D Resolution = FVector2D(512, 512);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Save")
		TEnumAsByte<enum TextureCompressionSettings> CompressionSettings = TextureCompressionSettings::TC_EditorIcon;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Save", meta = (DisplayName = "TextureGroup"))
		TEnumAsByte<enum TextureGroup> IconTextureGroup = TextureGroup::TEXTUREGROUP_UI;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview", meta = (AllowPrivateAccess = "true", DisplayThumbnail = "true", AllowedClasses = "/Script/Engine.Texture,/Script/Engine.MaterialInterface,/Script/Engine.SlateTextureAtlasInterface", DisallowedClasses = "/Script/MediaAssets.MediaTexture"))
		TObjectPtr<UObject> PreviewImage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Preview")
		FVector2D IconSize = FVector2D(50, 50);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Preview")
		EDFICBackgroundType Background;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Preview", meta = (EditCondition = "Background == EDFICBackgroundType::Color", EditConditionHides))
		FLinearColor Color;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Preview", meta = (EditCondition = "Background == EDFICBackgroundType::Texture", EditConditionHides))
		UTexture2D* Texture;

};
