// Source/DungeonForged/Private/Combat/AN/AN_TraceStart.cpp
#include "Combat/AN/AN_TraceStart.h"
#include "Components/SkeletalMeshComponent.h"
#include "Combat/UDFMeleeTraceComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "GameFramework/Actor.h"

void UAN_TraceStart::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
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
	if (UDFMeleeTraceComponent* T = Owner->FindComponentByClass<UDFMeleeTraceComponent>())
	{
		T->StartTrace();
	}
}
