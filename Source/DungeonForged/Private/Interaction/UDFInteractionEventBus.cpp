// Source/DungeonForged/Private/Interaction/UDFInteractionEventBus.cpp
#include "Interaction/UDFInteractionEventBus.h"
#include "Engine/World.h"

void UDFInteractionEventBus::DispatchGameplayEvent(const FGameplayTag EventTag, AActor* const Instigator)
{
	OnGameplayEvent.Broadcast(EventTag, Instigator);
}
