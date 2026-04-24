// Source/DungeonForged/Private/GameModes/Nexus/ADFRunPortal.cpp
#include "GameModes/Nexus/ADFRunPortal.h"
#include "GameModes/Nexus/ADFNexusPlayerController.h"
#include "GameModes/Nexus/ADFNexusGameState.h"
#include "NiagaraComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/DFInteractable.h"
#include "Engine/World.h"
#include "Math/Color.h"

ADFRunPortal::ADFRunPortal()
{
	bSingleUse = false;
	PortalVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PortalVFX"));
	PortalVFX->SetupAttachment(RootComponent);
	PortalVFX->SetAutoActivate(true);
	PortalLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PortalLight"));
	PortalLight->SetupAttachment(RootComponent);
	PortalLight->SetIntensity(1500.f);
	PortalLight->SetAttenuationRadius(1200.f);
}

void ADFRunPortal::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* const W = GetWorld())
	{
		if (ADFNexusGameState* const GS = W->GetGameState<ADFNexusGameState>())
		{
			SetPortalColorByMetaLevel(GS->MetaLevel);
		}
	}
}

void ADFRunPortal::SetPortalColorByMetaLevel(const int32 MetaLevel)
{
	if (!PortalLight)
	{
		return;
	}
	const uint8 B = (uint8)FMath::Clamp(180 + MetaLevel * 4, 0, 255);
	PortalLight->SetLightColor(FColor(100, 200, B, 255));
}

void ADFRunPortal::Interact_Implementation(ACharacter* const Interactor)
{
	if (!HasAuthority() || !Interactor)
	{
		return;
	}
	if (!IDFInteractable::Execute_CanInteract(this, Interactor))
	{
		return;
	}
	PlayInteractEffects(Interactor);
	if (APlayerController* const PC = Cast<APlayerController>(Interactor->GetController()))
	{
		if (ADFNexusPlayerController* const N = Cast<ADFNexusPlayerController>(PC))
		{
			N->Client_OpenClassSelection();
		}
	}
}
