#pragma once

#include "CoreMinimal.h"
#include "Engine/TextureDefines.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UDFEditorIconBakeLibrary.generated.h"

class USceneCaptureComponent2D;
class USkeletalMeshComponent;
class UTexture2D;
class UTextureRenderTarget2D;


UENUM(BlueprintType)
enum class EDFICAssetType : uint8
{
	StaticMesh UMETA(DisplayName = "Static Mesh"),
	SkeletalMesh UMETA(DisplayName = "Skeletal Mesh"),
	Actor
};

UENUM(BlueprintType)
enum class EDFICBackgroundType : uint8
{
	Transparent,
	Color,
	Texture
};

/** Preset de qualidade para textura final (separado de sistemas de gameplay). */
UENUM(BlueprintType)
enum class EDFIconBakeTexturePurpose : uint8
{
	/** 8-bit RGBA, foco em transparência para listas e HUD. */
	UiIcon UMETA(DisplayName = "Ícone UI (LDR, alphas)"),
	/** PF float / 16-bit no RT; menos compressão — splashes e perfil de hero. */
	HeroProfile UMETA(DisplayName = "Hero / splash (alta resolução HDR)"),
};

/**
 * Utilitários de Editor para gravar malhas (estáticas ou skeletal) vistas por SceneCapture2D
 * como Texture2D em /Game/..., sem depender de UDFClassSelectionSubsystem.
 * Uso típico: colocar BP com SceneCapture + meshes no mapa; EUW chama estes nós.
 */
UCLASS()
class DUNGEONFORGED_API UDFEditorIconBakeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Largura / altura são limitadas a um intervalo seguro (VRAM / limites de captura). */
	UFUNCTION(
		BlueprintCallable,
		Category = "DF|Editor|IconBake",
		meta = (DevelopmentOnly, WorldContext = "WorldContextObject",
			DisplayName = "DF Icon Bake · Configure RT e capturar"))
	static bool DFIconBake_CaptureSceneToRenderTarget(
		UObject* WorldContextObject,
		USceneCaptureComponent2D* SceneCapture,
		UTextureRenderTarget2D* RenderTarget,
		int32 Width,
		int32 Height,
		ETextureRenderTargetFormat RenderTargetFormat,
		FLinearColor BackgroundClearColor);

	/**
	 * Grava o conteúdo actual do RT como Texture2D no Content Browser.
	 * @param ContentFolder Caminho de conteúdo, ex.: /Game/DungeonForged/Tools/IconBake (sem trail slash final).
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "DF|Editor|IconBake",
		meta = (DevelopmentOnly, WorldContext = "WorldContextObject",
			DisplayName = "DF Icon Bake · RT → Texture2D Asset"))
	static UTexture2D* DFIconBake_SaveRenderTargetToTextureAsset(
		UObject* WorldContextObject,
		UTextureRenderTarget2D* RenderTarget,
		const FString& ContentFolder,
		const FString& AssetName,
		EDFIconBakeTexturePurpose Purpose);

	/**
	 * Grava o RT como Texture2D com compressão / mips / grupo explícitos (estilo ferramentas tipo Icon Creator).
	 * @param bCreateUniqueAssetNameIfExists Se true, evita falhar quando já existe — gera nome único (AssetTools).
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "DF|Editor|IconBake",
		meta = (DevelopmentOnly, WorldContext = "WorldContextObject",
			DisplayName = "DF Icon Bake · RT → Texture2D (definições manuais)"))
	static UTexture2D* DFIconBake_SaveRenderTargetToTextureAssetCustom(
		UObject* WorldContextObject,
		UTextureRenderTarget2D* RenderTarget,
		const FString& ContentFolder,
		const FString& AssetName,
		TEnumAsByte<TextureCompressionSettings> CompressionSettings,
		TEnumAsByte<TextureMipGenSettings> MipGenSettings,
		TEnumAsByte<TextureGroup> TextureLODGroup,
		bool bSRGB,
		bool bCreateUniqueAssetNameIfExists = false);

	/** Atalho: redesenha RT e grava textura num passo. */
	UFUNCTION(
		BlueprintCallable,
		Category = "DF|Editor|IconBake",
		meta = (DevelopmentOnly, WorldContext = "WorldContextObject",
			DisplayName = "DF Icon Bake · Capturar e gravar Texture2D"))
	static UTexture2D* DFIconBake_CaptureAndSaveTextureAsset(
		UObject* WorldContextObject,
		USceneCaptureComponent2D* SceneCapture,
		UTextureRenderTarget2D* RenderTarget,
		int32 Width,
		int32 Height,
		const FString& ContentFolder,
		const FString& AssetName,
		EDFIconBakeTexturePurpose Purpose,
		FLinearColor BackgroundClearColor);

	/** Mesmo que o atalho acima, mas formato do RT e metadados da textura escolhidos manualmente (ex. TC_EditorIcon + TEXTUREGROUP_UI). */
	UFUNCTION(
		BlueprintCallable,
		Category = "DF|Editor|IconBake",
		meta = (DevelopmentOnly, WorldContext = "WorldContextObject",
			DisplayName = "DF Icon Bake · Capturar e gravar Texture2D (definições manuais)"))
	static UTexture2D* DFIconBake_CaptureAndSaveTextureAssetCustom(
		UObject* WorldContextObject,
		USceneCaptureComponent2D* SceneCapture,
		UTextureRenderTarget2D* RenderTarget,
		int32 Width,
		int32 Height,
		const FString& ContentFolder,
		const FString& AssetName,
		ETextureRenderTargetFormat RenderTargetFormat,
		FLinearColor BackgroundClearColor,
		TEnumAsByte<TextureCompressionSettings> CompressionSettings,
		TEnumAsByte<TextureMipGenSettings> MipGenSettings,
		TEnumAsByte<TextureGroup> TextureLODGroup,
		bool bSRGB,
		bool bCreateUniqueAssetNameIfExists = false);

	/** Após alterar tempo de animação no Editor, força refresh do mesh (útil com SceneCapture). */
	UFUNCTION(
		BlueprintCallable,
		Category = "DF|Editor|IconBake",
		meta = (DevelopmentOnly, DisplayName = "DF Icon Bake · Atualizar preview skeletal (scrub)"))
	static void DFIconBake_RefreshSkeletalMeshPreview(USkeletalMeshComponent* SkeletalMeshComponent);
};
