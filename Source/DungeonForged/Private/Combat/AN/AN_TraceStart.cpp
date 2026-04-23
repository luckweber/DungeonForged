// Source/DungeonForged/Private/Combat/AN/AN_TraceStart.cpp
#include "Combat/AN/AN_TraceStart.h"
#include "Animation/AnimSequenceBase.h"
#include "Combat/UDFMeleeTraceComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void UAN_TraceStart::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
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
		return;
	}
	Trace->StartTrace();
}
