// Source/DungeonForged/Private/Animation/UDFAnimNotify_SpawnTrailVFX.cpp
#include "Animation/UDFAnimNotify_SpawnTrailVFX.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

void UUDFAnimNotify_SpawnTrailVFX::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	(void)Animation;
	(void)EventReference;
	if (!MeshComp)
	{
		return;
	}
	AActor* const RootOwner = MeshComp->GetOwner();
	TArray<UNiagaraComponent*> Niagaras;
	if (RootOwner)
	{
		RootOwner->GetComponents<UNiagaraComponent>(Niagaras, true);
		for (UNiagaraComponent* N : Niagaras)
		{
			if (N && (ComponentTag.IsNone() || N->ComponentHasTag(ComponentTag)))
			{
				N->Activate(true);
			}
		}
	}
	if (bSpawnIfMissing && Template)
	{
		bool bHasActive = false;
		for (UNiagaraComponent* N : Niagaras)
		{
			if (N && N->IsActive() && (ComponentTag.IsNone() || N->ComponentHasTag(ComponentTag)))
			{
				bHasActive = true;
				break;
			}
		}
		if (!bHasActive)
		{
			UNiagaraComponent* const Spawned = UNiagaraFunctionLibrary::SpawnSystemAttached(
				Template, MeshComp, AttachSocket, FVector::ZeroVector, FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget, true);
			if (Spawned)
			{
				if (!ComponentTag.IsNone())
				{
					Spawned->ComponentTags.AddUnique(ComponentTag);
				}
				Spawned->Activate(true);
			}
		}
	}
}
