// Source/DungeonForged/Private/Equipment/ADFEquipmentPreviewActor.cpp
#include "Equipment/ADFEquipmentPreviewActor.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Equipment/UDFPreviewCaptureComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Character.h"

ADFEquipmentPreviewActor::ADFEquipmentPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SetReplicatingMovement(false);

	PreviewMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewMesh"));
	SetRootComponent(PreviewMesh);
	PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMesh->SetComponentTickEnabled(false);
	PreviewMesh->SetCastShadow(true);
	PreviewMesh->SetHiddenInGame(false);

	Mesh_Helmet = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh_Helmet"));
	Mesh_Chest = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh_Chest"));
	Mesh_Legs = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh_Legs"));
	Mesh_Boots = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh_Boots"));
	Mesh_Gloves = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh_Gloves"));
	Mesh_Weapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh_Weapon"));
	Mesh_OffHand = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh_OffHand"));

	Mesh_Helmet->SetupAttachment(PreviewMesh);
	Mesh_Chest->SetupAttachment(PreviewMesh);
	Mesh_Legs->SetupAttachment(PreviewMesh);
	Mesh_Boots->SetupAttachment(PreviewMesh);
	Mesh_Gloves->SetupAttachment(PreviewMesh);
	static const FName NWeaponR(TEXT("weapon_r"));
	static const FName NWeaponL(TEXT("weapon_l"));
	if (PreviewMesh->DoesSocketExist(NWeaponR))
	{
		Mesh_Weapon->SetupAttachment(PreviewMesh, NWeaponR);
	}
	else
	{
		Mesh_Weapon->SetupAttachment(PreviewMesh);
	}
	if (PreviewMesh->DoesSocketExist(NWeaponL))
	{
		Mesh_OffHand->SetupAttachment(PreviewMesh, NWeaponL);
	}
	else
	{
		Mesh_OffHand->SetupAttachment(PreviewMesh);
	}

	ApplyModularDefaults();

	PreviewCapture = CreateDefaultSubobject<UDFPreviewCaptureComponent>(TEXT("PreviewCapture"));
	PreviewCapture->SetupAttachment(PreviewMesh);
	PreviewCapture->SetRelativeLocation(FVector::ZeroVector);
	PreviewCapture->SetUsingAbsoluteRotation(true);
}

void ADFEquipmentPreviewActor::ApplyModularDefaults()
{
	auto ConfigPart = [](USkeletalMeshComponent* Part)
	{
		if (!Part)
		{
			return;
		}
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetComponentTickEnabled(false);
		Part->bReceivesDecals = true;
		Part->SetCastShadow(true);
	};
	ConfigPart(Mesh_Helmet);
	ConfigPart(Mesh_Chest);
	ConfigPart(Mesh_Legs);
	ConfigPart(Mesh_Boots);
	ConfigPart(Mesh_Gloves);
	ConfigPart(Mesh_Weapon);
	ConfigPart(Mesh_OffHand);
}

void ADFEquipmentPreviewActor::BeginPlay()
{
	Super::BeginPlay();
	SetActorEnableCollision(false);
}

void ADFEquipmentPreviewActor::CopySkelVisual(
	USkeletalMeshComponent* const Dest,
	USkeletalMeshComponent* const Src,
	USkeletalMeshComponent* const LeaderForSlaves)
{
	if (!Dest)
	{
		return;
	}
	if (!Src)
	{
		Dest->SetSkeletalMesh(nullptr);
		Dest->SetAnimInstanceClass(nullptr);
		Dest->SetLeaderPoseComponent(nullptr);
		return;
	}
	Dest->SetSkeletalMesh(Src->GetSkeletalMeshAsset());
	Dest->SetAnimInstanceClass(Src->GetAnimClass());
	if (LeaderForSlaves && Dest != LeaderForSlaves)
	{
		Dest->SetLeaderPoseComponent(LeaderForSlaves, true);
	}
	else
	{
		Dest->SetLeaderPoseComponent(nullptr);
	}
}

void ADFEquipmentPreviewActor::SyncFromDFPlayer(ADFPlayerCharacter* const Source)
{
	if (!Source || !PreviewMesh)
	{
		return;
	}
	USkeletalMeshComponent* SrcBase = Source->Mesh_Base.Get();
	if (!SrcBase)
	{
		SrcBase = Source->GetMesh();
	}
	if (!SrcBase)
	{
		return;
	}
	CopySkelVisual(PreviewMesh, SrcBase, nullptr);
	CopySkelVisual(Mesh_Helmet, Source->Mesh_Helmet, PreviewMesh);
	CopySkelVisual(Mesh_Chest, Source->Mesh_Chest, PreviewMesh);
	CopySkelVisual(Mesh_Legs, Source->Mesh_Legs, PreviewMesh);
	CopySkelVisual(Mesh_Boots, Source->Mesh_Boots, PreviewMesh);
	CopySkelVisual(Mesh_Gloves, Source->Mesh_Gloves, PreviewMesh);
	CopySkelVisual(Mesh_Weapon, Source->Mesh_Weapon, PreviewMesh);
	CopySkelVisual(Mesh_OffHand, Source->Mesh_OffHand, PreviewMesh);
}

void ADFEquipmentPreviewActor::SyncMeshFromCharacter(ACharacter* const SourceCharacter)
{
	if (!SourceCharacter || !PreviewMesh)
	{
		return;
	}
	if (ADFPlayerCharacter* const DF = Cast<ADFPlayerCharacter>(SourceCharacter))
	{
		SyncFromDFPlayer(DF);
		return;
	}
	USkeletalMeshComponent* const SrcMesh = SourceCharacter->GetMesh();
	if (!SrcMesh)
	{
		return;
	}
	CopySkelVisual(PreviewMesh, SrcMesh, nullptr);
}

void ADFEquipmentPreviewActor::InitializePreview(UTextureRenderTarget2D* const RenderTarget)
{
	if (!PreviewCapture || !RenderTarget)
	{
		return;
	}
	PreviewCapture->SetRenderTargetForCapture(RenderTarget);
	PreviewCapture->SetPreviewTarget(this);
}

void ADFEquipmentPreviewActor::SetPreviewActive(const bool bActive)
{
	if (PreviewCapture)
	{
		PreviewCapture->SetPreviewActive(bActive);
	}
}

void ADFEquipmentPreviewActor::AddOrbitDeltaYaw(const float DeltaDegrees)
{
	if (PreviewCapture)
	{
		PreviewCapture->AddOrbitDeltaYaw(DeltaDegrees);
	}
}
