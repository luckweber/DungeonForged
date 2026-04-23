// Source/DungeonForged/Private/DungeonForgedGameInstance.cpp
#include "DungeonForgedGameInstance.h"
#include "AbilitySystemGlobals.h"
#include "GAS/DFGameplayTags.h"

void UDungeonForgedGameInstance::Init()
{
	Super::Init();
	FDFGameplayTags::RegisterGameplayTags();
	UAbilitySystemGlobals::Get().InitGlobalData();
}
