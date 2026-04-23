// Source/DungeonForged/Private/Animation/UDFAnimNotify_DisableTrailVFX.cpp
#include "Animation/UDFAnimNotify_DisableTrailVFX.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"

void UUDFAnimNotify_DisableTrailVFX::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	(void)Animation;
	(void)EventReference;
	if (!MeshComp)
	{
		return;
	}
	AActor* const Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}
	TArray<UNiagaraComponent*> Ns;
	Owner->GetComponents<UNiagaraComponent>(Ns, true);
	for (UNiagaraComponent* N : Ns)
	{
		if (N && (ComponentTag.IsNone() || N->ComponentHasTag(ComponentTag)))
		{
			N->Deactivate();
		}
	}
}
