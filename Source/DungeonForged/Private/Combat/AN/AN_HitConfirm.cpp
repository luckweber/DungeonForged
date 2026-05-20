// Source/DungeonForged/Private/Combat/AN/AN_HitConfirm.cpp
#include "Combat/AN/AN_HitConfirm.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Combat/UDFMeleeTraceComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GameFramework/Actor.h"

FString UAN_HitConfirm::GetNotifyName_Implementation() const
{
	return TEXT("DF Hit Confirm");
}

void UAN_HitConfirm::Notify(USkeletalMeshComponent* const MeshComp, UAnimSequenceBase* const Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	AActor* const Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}
	if (FDFGameplayTags::Event_Combat_HitConfirm.IsValid())
	{
		FGameplayEventData Payload;
		Payload.EventTag = FDFGameplayTags::Event_Combat_HitConfirm;
		Payload.Instigator = Owner;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Owner, FDFGameplayTags::Event_Combat_HitConfirm, Payload);
	}
	if (bForceTracePulse)
	{
		if (UDFMeleeTraceComponent* const Trace = Owner->FindComponentByClass<UDFMeleeTraceComponent>())
		{
			if (Trace->bTracing)
			{
				Trace->TickTrace(0.f);
			}
		}
	}
}
