#include "GAS/Effects/UGE_Cooldown_Universal_Berserk.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Cooldown_Universal_Berserk::UGE_Cooldown_Universal_Berserk()
{
	CooldownAssociatedAbilityTag = FDFGameplayTags::Ability_Universal_Berserk;
}

void UGE_Cooldown_Universal_Berserk::ConfigureEffectCDO()
{
	Super::ConfigureEffectCDO();
	UTargetTagsGameplayEffectComponent& Gr = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Tags;
	Tags.AddTag(FDFGameplayTags::Ability_Cooldown_Berserk);
	Gr.SetAndApplyTargetTagChanges(Tags);
}
