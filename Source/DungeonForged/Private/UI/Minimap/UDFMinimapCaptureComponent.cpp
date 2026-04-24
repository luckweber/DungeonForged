// Source/DungeonForged/Private/UI/Minimap/UDFMinimapCaptureComponent.cpp
#include "UI/Minimap/UDFMinimapCaptureComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/EngineTypes.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ShowFlags.h"
#include "TimerManager.h"

UDFMinimapCaptureComponent::UDFMinimapCaptureComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bTickInEditor = false;

	CaptureComp = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MinimapSceneCapture2D"));
	CaptureComp->SetupAttachment(this);
}

void UDFMinimapCaptureComponent::BeginPlay()
{
	Super::BeginPlay();

	UWorld* const W = GetWorld();
	if (!W || !W->IsGameWorld())
	{
		return;
	}

	ApplyDefaultCaptureSettings();
	if (MinimapRenderTarget)
	{
		CaptureComp->TextureTarget = MinimapRenderTarget;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UDFMinimapCaptureComponent: MinimapRenderTarget is not set; capture is disabled until assigned."));
	}
	StartUpdateTimer();
}

void UDFMinimapCaptureComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopUpdateTimer();
	Super::EndPlay(EndPlayReason);
}

void UDFMinimapCaptureComponent::ApplyDefaultCaptureSettings()
{
	if (!CaptureComp)
	{
		return;
	}
	CaptureComp->ProjectionType = ECameraProjectionMode::Orthographic;
	CaptureComp->OrthoWidth = BaseOrthoWidth;
	CaptureComp->bCaptureEveryFrame = false;
	CaptureComp->bCaptureOnMovement = false;
	CaptureComp->bAlwaysPersistRenderingState = false;
	CaptureComp->CaptureSource = SCS_FinalColorHDR;
	CaptureComp->PrimitiveRenderMode = bFilterWithShowOnly
		? ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList
		: ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;

	// Minimap: disable expensive show flags; reflection/fog/atmospheric effects are not needed.
	FEngineShowFlags& Sf = CaptureComp->ShowFlags;
	Sf.SetDynamicShadows(false);
	Sf.SetFog(false);
	Sf.SetVolumetricFog(false);
	Sf.SetAtmosphere(false);
	Sf.SetScreenSpaceReflections(false);
	Sf.SetLumenReflections(false);
	Sf.SetLumenGlobalIllumination(false);
	Sf.SetReflectionEnvironment(false);
}

void UDFMinimapCaptureComponent::UpdatePosition(const FVector PlayerLocation)
{
	const FVector P(PlayerLocation.X, PlayerLocation.Y, PlayerLocation.Z + HeightAbovePlayer);
	SetWorldLocation(P);
	// Top-down ortho: look straight down. Adjust in Blueprint if the RT needs a yaw offset.
	SetWorldRotation(FRotator(-90.0, 0.0, 0.0));
}

void UDFMinimapCaptureComponent::SetOrthoWidth(const float Width)
{
	if (CaptureComp)
	{
		CaptureComp->OrthoWidth = FMath::Max(10.f, Width);
	}
}

void UDFMinimapCaptureComponent::HandleUpdateTimer()
{
	if (const TWeakObjectPtr<AActor> P = TrackedPawn; P.IsValid())
	{
		UpdatePosition(P->GetActorLocation());
	}
	Recapture();
}

void UDFMinimapCaptureComponent::Recapture()
{
	if (!MinimapRenderTarget || !CaptureComp)
	{
		return;
	}
	CaptureComp->TextureTarget = MinimapRenderTarget;
	CaptureComp->CaptureScene();
}

void UDFMinimapCaptureComponent::StartUpdateTimer()
{
	if (UWorld* const W = GetWorld())
	{
		if (!W->IsGameWorld() || !MinimapRenderTarget)
		{
			return;
		}
		W->GetTimerManager().SetTimer(
			UpdateTimer,
			this,
			&UDFMinimapCaptureComponent::HandleUpdateTimer,
			UpdateInterval,
			true);
	}
}

void UDFMinimapCaptureComponent::StopUpdateTimer()
{
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(UpdateTimer);
	}
}
