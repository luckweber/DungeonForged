// Source/DungeonForged/Private/Combat/AN/AN_TraceEnd.cpp
#include "Combat/AN/AN_TraceEnd.h"
#include "Animation/AnimSequenceBase.h"
#include "Combat/UDFMeleeTraceComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void UAN_TraceEnd::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	(void)Animation;
	(void)EventReference;
	if (!IsValid(MeshComp))
	{
		return;
	}
	AActor* const Owner = MeshComp->GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}
	UDFMeleeTraceComponent* const Trace = Owner->FindComponentByClass<UDFMeleeTraceComponent>();
	if (!IsValid(Trace))
	{
		// e.g. Persona, montage on enemy without MeleeTrace, or child BP with no component
		return;
	}
	Trace->EndTrace();
}
