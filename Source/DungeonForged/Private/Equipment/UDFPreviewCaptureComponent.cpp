// Source/DungeonForged/Private/Equipment/UDFPreviewCaptureComponent.cpp
#include "Equipment/UDFPreviewCaptureComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/UObjectGlobals.h"

UDFPreviewCaptureComponent::UDFPreviewCaptureComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bTickInEditor = false;
	// Do not CreateDefaultSubobject a nested SceneCapture2D: Blueprint subclasses (e.g. BP_HeroCharacter) can duplicate
	// a child of the CDO and attach to an instanced parent (SceneComponent.cpp ~2105 "Template Mismatch").
}

void UDFPreviewCaptureComponent::EnsureRuntimeSceneCapture()
{
	if (SceneCapture)
	{
		return;
	}
	AActor* const Owner = GetOwner();
	if (!Owner || Owner->HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}
	UWorld* const World = GetWorld() ? GetWorld() : Owner->GetWorld();
	if (!World)
	{
		return;
	}

	// Name must not clash with a stale default subobject in old BP_hero uassets.
	const FName CapName = MakeUniqueObjectName(this, USceneCaptureComponent2D::StaticClass(), TEXT("DFSceneCapture2D"));
	SceneCapture = NewObject<USceneCaptureComponent2D>(this, USceneCaptureComponent2D::StaticClass(), CapName, RF_Transactional);
	if (!SceneCapture)
	{
		return;
	}
	SceneCapture->SetRelativeLocation(FVector(200.f, 0.f, 80.f));
	SceneCapture->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = false;
	SceneCapture->bAlwaysPersistRenderingState = false;
	SceneCapture->SetupAttachment(this);
	if (!SceneCapture->IsRegistered())
	{
		SceneCapture->RegisterComponent();
	}
}

void UDFPreviewCaptureComponent::OnRegister()
{
	Super::OnRegister();
	EnsureRuntimeSceneCapture();
	ApplyOrbit();
	if (SceneCapture)
	{
		SceneCapture->ProjectionType = ECameraProjectionMode::Orthographic;
		SceneCapture->OrthoWidth = OrthoWidth;
		if (RenderTarget)
		{
			SceneCapture->TextureTarget = RenderTarget;
		}
	}
}

void UDFPreviewCaptureComponent::BeginPlay()
{
	Super::BeginPlay();
	// In case OnRegister had no world yet, create capture for paper-doll before UI opens.
	EnsureRuntimeSceneCapture();
	if (SceneCapture)
	{
		SceneCapture->ProjectionType = ECameraProjectionMode::Orthographic;
		SceneCapture->OrthoWidth = OrthoWidth;
		if (RenderTarget)
		{
			SceneCapture->TextureTarget = RenderTarget;
		}
	}
	ApplyOrbit();
}

void UDFPreviewCaptureComponent::ApplyOrbit()
{
	SetRelativeRotation(FRotator(0.f, 180.f + OrbitYawDegrees, 0.f));
}

void UDFPreviewCaptureComponent::SetPreviewTarget(AActor* const OnlyShow)
{
	if (!SceneCapture)
	{
		return;
	}
	SceneCapture->ShowOnlyActors.Reset();
	if (IsValid(OnlyShow))
	{
		SceneCapture->ShowOnlyActors.Add(OnlyShow);
	}
}

void UDFPreviewCaptureComponent::SetPreviewActive(const bool bActive)
{
	if (!SceneCapture)
	{
		return;
	}
	SceneCapture->bCaptureEveryFrame = bActive;
	SceneCapture->bCaptureOnMovement = bActive;
	if (bActive)
	{
		SceneCapture->CaptureScene();
	}
}

void UDFPreviewCaptureComponent::AddOrbitDeltaYaw(const float DeltaDegrees)
{
	OrbitYawDegrees += DeltaDegrees;
	ApplyOrbit();
}

void UDFPreviewCaptureComponent::SetRenderTargetForCapture(UTextureRenderTarget2D* const RT)
{
	RenderTarget = RT;
	if (SceneCapture)
	{
		SceneCapture->TextureTarget = RT;
	}
}
