// Source/DungeonForged/Private/Animation/UDFAnimNotify_FootStep.cpp
#include "Animation/UDFAnimNotify_FootStep.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundBase.h"

void UUDFAnimNotify_FootStep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	(void)Animation;
	(void)EventReference;
	if (!MeshComp || !MeshComp->GetWorld())
	{
		return;
	}
	const FVector Base = !FootSocketName.IsNone() && MeshComp->DoesSocketExist(FootSocketName)
		? MeshComp->GetSocketLocation(FootSocketName)
		: MeshComp->GetComponentLocation();
	const FVector Start = Base + FVector(0.f, 0.f, TraceUp);
	const FVector End = Base - FVector(0.f, 0.f, TraceUp + TraceDown);
	FHitResult Hit;
	FCollisionQueryParams P(SCENE_QUERY_STAT(DF_FootStep), true, MeshComp->GetOwner());
	AActor* Owner = MeshComp->GetOwner();
	if (Owner)
	{
		P.AddIgnoredActor(Owner);
	}
	USoundBase* ToPlay = DefaultSound;
	if (MeshComp->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, P) && Hit.bBlockingHit)
	{
		if (const UPhysicalMaterial* const PM = Hit.PhysMaterial.Get())
		{
			// TEnumAsByte on UPhysicalMaterial; map to your project's Stone/Dirt/Wood/Metal in notify defaults
			const EPhysicalSurface S = (EPhysicalSurface)PM->SurfaceType;
			if (S == (EPhysicalSurface)MapStoneTo && Sound_Stone) { ToPlay = Sound_Stone; }
			else if (S == (EPhysicalSurface)MapDirtTo && Sound_Dirt) { ToPlay = Sound_Dirt; }
			else if (S == (EPhysicalSurface)MapWoodTo && Sound_Wood) { ToPlay = Sound_Wood; }
			else if (S == (EPhysicalSurface)MapMetalTo && Sound_Metal) { ToPlay = Sound_Metal; }
		}
	}
	if (ToPlay)
	{
		UGameplayStatics::PlaySoundAtLocation(MeshComp, ToPlay, Base);
	}
}
