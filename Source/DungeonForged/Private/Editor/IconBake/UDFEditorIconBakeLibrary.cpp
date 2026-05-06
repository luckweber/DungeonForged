#include "Editor/IconBake/UDFEditorIconBakeLibrary.h"
#include "DungeonForgedModule.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "IAssetTools.h"
#include "RenderingThread.h"
#include "UObject/UObjectGlobals.h"
#endif

namespace
{
constexpr int32 IconBakeMinRes = 8;
constexpr int32 IconBakeMaxRes = 8192;

static FString DfSanitizeAssetName(FString In)
{
	In.TrimStartAndEndInline();
	for (TCHAR& C : In)
	{
		if (C == TCHAR(' ') || C == TCHAR('/') || C == TCHAR('\\') || C == TCHAR(':') || C == TCHAR('.'))
		{
			C = TCHAR('_');
		}
	}
	return In.IsEmpty() ? FString(TEXT("T_NewIcon")) : In;
}

static FString DfNormalizeContentFolder(FString Folder)
{
	Folder.TrimStartAndEndInline();
	Folder.ReplaceInline(TEXT("\\"), TEXT("/"));
	if (!Folder.StartsWith(TEXT("/Game/")))
	{
		FString Tail = Folder;
		Tail.RemoveFromStart(TEXT("/"));
		Tail.RemoveFromStart(TEXT("Game/"));
		if (!Tail.StartsWith(TEXT("Game/")))
		{
			Folder = FString::Printf(TEXT("/Game/%s"), *Tail);
		}
		else
		{
			Folder = FString(TEXT("/")) + Tail;
		}
	}
	while (Folder.Len() > 1 && Folder.EndsWith(TEXT("/")))
	{
		Folder.LeftChopInline(1);
	}
	return Folder;
}

#if WITH_EDITOR

static UWorld* DfResolveEditorWorld(UObject* const WorldContextObject, USceneCaptureComponent2D* const Capture)
{
	if (WorldContextObject)
	{
		if (UWorld* const W = WorldContextObject->GetWorld())
		{
			return W;
		}
	}
	if (Capture)
	{
		if (UWorld* const W = Capture->GetWorld())
		{
			return W;
		}
	}
	if (GEditor)
	{
		return GEditor->GetEditorWorldContext().World();
	}
	return nullptr;
}

static bool DfTextureAlreadyExists(const FString& PackageName, const FName ObjectName)
{
	const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackageName, *ObjectName.ToString());
	return StaticFindObject(UTexture2D::StaticClass(), nullptr, *ObjectPath) != nullptr;
}

/** Resolve pacote + nome do objeto; falha só por colisão quando @a bUniqueName é false. */
static bool DfResolveIconBakeDestination(
	const FString& ContentFolder,
	const FString& AssetName,
	bool bCreateUniqueAssetNameIfExists,
	FString& OutPackagePath,
	FName& OutObjectName)
{
	const FString Folder = DfNormalizeContentFolder(ContentFolder);
	const FString SafeNameStr = DfSanitizeAssetName(AssetName);
	if (bCreateUniqueAssetNameIfExists)
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		const FString FullPath = FString::Printf(TEXT("%s/%s"), *Folder, *SafeNameStr);
		FString UniquePackage;
		FString UniqueAssetName;
		AssetTools.CreateUniqueAssetName(FullPath, FString(), UniquePackage, UniqueAssetName);
		OutPackagePath = MoveTemp(UniquePackage);
		OutObjectName = FName(*UniqueAssetName);
		return true;
	}

	OutPackagePath = FString::Printf(TEXT("%s/%s"), *Folder, *SafeNameStr);
	OutObjectName = FName(*SafeNameStr);
	return !DfTextureAlreadyExists(OutPackagePath, OutObjectName);
}

static UTexture2D* DfSaveRenderTargetAsTexture2D(
	UWorld* World,
	UTextureRenderTarget2D* RenderTarget,
	const FString& ResolvedPackagePath,
	const FName ObjectName,
	TextureCompressionSettings Compression,
	TextureMipGenSettings Mips,
	TextureGroup Group,
	bool bSRGB)
{
	UPackage* const Pkg = CreatePackage(*ResolvedPackagePath);
	if (!Pkg)
	{
		DF_LOG(Warning, "[DF|IconBake] CreatePackage falhou: %s", *ResolvedPackagePath);
		return nullptr;
	}

	UTexture2D* const DstTexture = NewObject<UTexture2D>(Pkg, ObjectName,
		RF_Public | RF_Standalone | RF_Transactional);
	if (!DstTexture)
	{
		DF_LOG(Warning, "[DF|IconBake] NewObject Texture2D falhou.");
		return nullptr;
	}

	DstTexture->CompressionSettings = Compression;
	DstTexture->MipGenSettings = Mips;
	DstTexture->LODGroup = Group;
	DstTexture->SRGB = bSRGB;

	UKismetRenderingLibrary::ConvertRenderTargetToTexture2DEditorOnly(World, RenderTarget, DstTexture);

	DstTexture->PostEditChange();
	DstTexture->MarkPackageDirty();
	Pkg->MarkPackageDirty();

	FAssetRegistryModule::AssetCreated(DstTexture);

	TArray<UPackage*> ToSave;
	ToSave.Add(Pkg);
	if (!UEditorLoadingAndSavingUtils::SavePackages(ToSave, false))
	{
		DF_LOG(Warning, "[DF|IconBake] SavePackages falhou (ver AssetName / pasta gravável): %s", *ResolvedPackagePath);
		return nullptr;
	}

	DF_LOG(Log, "[DF|IconBake] Texture gravada: %s.%s", *ResolvedPackagePath, *ObjectName.ToString());
	return DstTexture;
}

#endif

} // namespace

bool UDFEditorIconBakeLibrary::DFIconBake_CaptureSceneToRenderTarget(UObject* WorldContextObject,
	USceneCaptureComponent2D* SceneCapture,
	UTextureRenderTarget2D* RenderTarget,
	int32 Width,
	int32 Height,
	ETextureRenderTargetFormat RenderTargetFormat,
	FLinearColor BackgroundClearColor)
{
#if WITH_EDITOR
	UWorld* World = DfResolveEditorWorld(WorldContextObject, SceneCapture);
	if (!World || !SceneCapture || !RenderTarget)
	{
		DF_LOG(Warning, "[DF|IconBake] Capture: World, SceneCapture ou RenderTarget inválido.");
		return false;
	}
	Width = FMath::Clamp(Width, IconBakeMinRes, IconBakeMaxRes);
	Height = FMath::Clamp(Height, IconBakeMinRes, IconBakeMaxRes);

	RenderTarget->bAutoGenerateMips = false;
	RenderTarget->RenderTargetFormat = RenderTargetFormat;
	RenderTarget->ClearColor = BackgroundClearColor;
	RenderTarget->ResizeTarget(static_cast<uint32>(Width), static_cast<uint32>(Height));
	RenderTarget->UpdateResourceImmediate(true);

	UKismetRenderingLibrary::ClearRenderTarget2D(World, RenderTarget, BackgroundClearColor);

	SceneCapture->TextureTarget = RenderTarget;
	SceneCapture->CaptureScene();
	FlushRenderingCommands();
	return true;
#else
	return false;
#endif
}

UTexture2D* UDFEditorIconBakeLibrary::DFIconBake_SaveRenderTargetToTextureAsset(UObject* WorldContextObject,
	UTextureRenderTarget2D* RenderTarget,
	const FString& ContentFolder,
	const FString& AssetName,
	EDFIconBakeTexturePurpose Purpose)
{
#if WITH_EDITOR
	UWorld* World = DfResolveEditorWorld(WorldContextObject, nullptr);
	if (!World || !RenderTarget)
	{
		DF_LOG(Warning, "[DF|IconBake] Save: World ou RenderTarget inválido.");
		return nullptr;
	}

	FString PackagePath;
	FName ObjectName(NAME_None);
	if (!DfResolveIconBakeDestination(ContentFolder, AssetName, false, PackagePath, ObjectName))
	{
		DF_LOG(Warning, "[DF|IconBake] Já existe %s — apaga, muda AssetName, ou usa nó ‟definições manuais\" com nome único.",
			*PackagePath);
		return nullptr;
	}

	TextureCompressionSettings Compression = TC_BC7;
	TextureMipGenSettings Mips = TMGS_NoMipmaps;
	TextureGroup Group = TEXTUREGROUP_UI;
	bool bSRGB = true;
	switch (Purpose)
	{
	case EDFIconBakeTexturePurpose::HeroProfile:
		Compression = TC_HDR;
		Mips = TMGS_NoMipmaps;
		Group = TEXTUREGROUP_Cinematic;
		bSRGB = false;
		break;
	default:
	case EDFIconBakeTexturePurpose::UiIcon:
		break;
	}

	return DfSaveRenderTargetAsTexture2D(
		World, RenderTarget, PackagePath, ObjectName, Compression, Mips, Group, bSRGB);
#else
	return nullptr;
#endif
}

UTexture2D* UDFEditorIconBakeLibrary::DFIconBake_SaveRenderTargetToTextureAssetCustom(UObject* WorldContextObject,
	UTextureRenderTarget2D* RenderTarget,
	const FString& ContentFolder,
	const FString& AssetName,
	TEnumAsByte<TextureCompressionSettings> CompressionSettings,
	TEnumAsByte<TextureMipGenSettings> MipGenSettings,
	TEnumAsByte<TextureGroup> TextureLODGroup,
	bool bSRGB,
	bool bCreateUniqueAssetNameIfExists)
{
#if WITH_EDITOR
	UWorld* World = DfResolveEditorWorld(WorldContextObject, nullptr);
	if (!World || !RenderTarget)
	{
		DF_LOG(Warning, "[DF|IconBake] Save Custom: World ou RenderTarget inválido.");
		return nullptr;
	}

	FString PackagePath;
	FName ObjectName(NAME_None);
	if (!DfResolveIconBakeDestination(ContentFolder, AssetName, bCreateUniqueAssetNameIfExists, PackagePath,
			ObjectName))
	{
		const FString FolderDbg = DfNormalizeContentFolder(ContentFolder);
		const FString NameDbg = DfSanitizeAssetName(AssetName);
		DF_LOG(Warning, "[DF|IconBake] Caminho ocupado ou inválido: %s/%s.", *FolderDbg, *NameDbg);
		return nullptr;
	}

	return DfSaveRenderTargetAsTexture2D(World, RenderTarget, PackagePath, ObjectName,
		static_cast<TextureCompressionSettings>(CompressionSettings.GetValue()),
		static_cast<TextureMipGenSettings>(MipGenSettings.GetValue()),
		static_cast<TextureGroup>(TextureLODGroup.GetValue()), bSRGB);
#else
	return nullptr;
#endif
}

UTexture2D* UDFEditorIconBakeLibrary::DFIconBake_CaptureAndSaveTextureAsset(UObject* WorldContextObject,
	USceneCaptureComponent2D* SceneCapture,
	UTextureRenderTarget2D* RenderTarget,
	int32 Width,
	int32 Height,
	const FString& ContentFolder,
	const FString& AssetName,
	EDFIconBakeTexturePurpose Purpose,
	FLinearColor BackgroundClearColor)
{
	const ETextureRenderTargetFormat RTF =
		(Purpose == EDFIconBakeTexturePurpose::HeroProfile) ? RTF_RGBA16f : RTF_RGBA8_SRGB;
	if (!DFIconBake_CaptureSceneToRenderTarget(
			WorldContextObject, SceneCapture, RenderTarget, Width, Height, RTF, BackgroundClearColor))
	{
		return nullptr;
	}
	return DFIconBake_SaveRenderTargetToTextureAsset(
		WorldContextObject, RenderTarget, ContentFolder, AssetName, Purpose);
}

UTexture2D* UDFEditorIconBakeLibrary::DFIconBake_CaptureAndSaveTextureAssetCustom(UObject* WorldContextObject,
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
	bool bCreateUniqueAssetNameIfExists)
{
	if (!DFIconBake_CaptureSceneToRenderTarget(WorldContextObject, SceneCapture, RenderTarget, Width, Height,
			RenderTargetFormat, BackgroundClearColor))
	{
		return nullptr;
	}
	return DFIconBake_SaveRenderTargetToTextureAssetCustom(WorldContextObject, RenderTarget, ContentFolder, AssetName,
		CompressionSettings, MipGenSettings, TextureLODGroup, bSRGB, bCreateUniqueAssetNameIfExists);
}

void UDFEditorIconBakeLibrary::DFIconBake_RefreshSkeletalMeshPreview(USkeletalMeshComponent* SkeletalMeshComponent)
{
#if WITH_EDITOR
	if (!SkeletalMeshComponent)
	{
		return;
	}

	SkeletalMeshComponent->RefreshBoneTransforms();
	SkeletalMeshComponent->UpdateComponentToWorld();
	SkeletalMeshComponent->FinalizeBoneTransform();
	SkeletalMeshComponent->MarkRenderTransformDirty();
	SkeletalMeshComponent->MarkRenderDynamicDataDirty();
#endif
}
