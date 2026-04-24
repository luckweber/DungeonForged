// Source/DungeonForged/Private/Animation/UDFAnimNotify_FootStep.cpp
#include "Animation/UDFAnimNotify_FootStep.h"
#include "Audio/UDFAudioComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundBase.h"

UUDFAnimNotify_FootStep::UUDFAnimNotify_FootStep()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(100, 255, 100);
	bShouldFireInEditor = false;
#endif
}

FString UUDFAnimNotify_FootStep::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("DF Footstep (%s)"), *FootSocketName.ToString());
}

void UUDFAnimNotify_FootStep::Notify(
	USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const /*Animation*/,
	const FAnimNotifyEventReference& /*EventReference*/)
{
	if (!MeshComp)
	{
		return;
	}
	UWorld* const W = MeshComp->GetWorld();
	if (!W || W->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	AActor* const Owner = MeshComp->GetOwner();
	const FVector Base = !FootSocketName.IsNone() && MeshComp->DoesSocketExist(FootSocketName)
		? MeshComp->GetSocketLocation(FootSocketName)
		: MeshComp->GetComponentLocation();
	const FVector Start = Base + FVector(0.f, 0.f, TraceUp);
	const FVector End = Base - FVector(0.f, 0.f, TraceUp + TraceDown);
	FHitResult Hit;
	FCollisionQueryParams P(SCENE_QUERY_STAT(DF_FootStep), true, Owner);
	P.bReturnPhysicalMaterial = true;
	USoundBase* LegacyToPlay = DefaultSound;
	TEnumAsByte<EPhysicalSurface> Surface = SurfaceType_Default;
	bool bHit = false;
	if (W->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, P) && Hit.bBlockingHit)
	{
		bHit = true;
		if (const UPhysicalMaterial* const PM = Hit.PhysMaterial.Get())
		{
			Surface = PM->SurfaceType;
			// Legacy inline selection
			const EPhysicalSurface S = (EPhysicalSurface)PM->SurfaceType;
			if (S == (EPhysicalSurface)MapStoneTo && Sound_Stone) { LegacyToPlay = Sound_Stone; }
			else if (S == (EPhysicalSurface)MapDirtTo && Sound_Dirt) { LegacyToPlay = Sound_Dirt; }
			else if (S == (EPhysicalSurface)MapWoodTo && Sound_Wood) { LegacyToPlay = Sound_Wood; }
			else if (S == (EPhysicalSurface)MapMetalTo && Sound_Metal) { LegacyToPlay = Sound_Metal; }
		}
	}

	if (Owner)
	{
		if (UDFAudioComponent* const Audio = Owner->FindComponentByClass<UDFAudioComponent>())
		{
			if (bHit)
			{
				Audio->PlayFootstep(Surface, Hit.ImpactPoint);
			}
			else
			{
				Audio->PlayFootstep(Surface, Base);
			}
			if (bHit)
			{
				if (const TObjectPtr<UNiagaraSystem>* Vfx = FootstepVfxBySurface.Find(Surface))
				{
					if (Vfx->Get())
					{
						UNiagaraFunctionLibrary::SpawnSystemAtLocation(
							W, Vfx->Get(), Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
					}
				}
			}
			return;
		}
	}
	if (LegacyToPlay)
	{
		UGameplayStatics::PlaySoundAtLocation(MeshComp, LegacyToPlay, bHit ? Hit.ImpactPoint : Base);
	}
	if (bHit)
	{
		if (const TObjectPtr<UNiagaraSystem>* Vfx = FootstepVfxBySurface.Find(Surface))
		{
			if (Vfx->Get())
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					W, Vfx->Get(), Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
			}
		}
	}
}
