// Source/DungeonForged/Private/Combat/AN/AN_SendGameplayEvent.cpp
#include "Combat/AN/AN_SendGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

FString UAN_SendGameplayEvent::GetNotifyName_Implementation() const
{
	return EventTag.IsValid()
		? FString::Printf(TEXT("DF Event: %s"), *EventTag.ToString())
		: FString(TEXT("DF Send Gameplay Event"));
}

void UAN_SendGameplayEvent::Notify(
	USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation,
	const FAnimNotifyEventReference& EventReference)
{
	(void)Animation;
	(void)EventReference;
	if (!IsValid(MeshComp) || !EventTag.IsValid())
	{
		return;
	}

	AActor* const Owner = MeshComp->GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = Owner;
	Payload.Target = Owner;
	Payload.EventMagnitude = EventMagnitude;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EventTag, Payload);
}
