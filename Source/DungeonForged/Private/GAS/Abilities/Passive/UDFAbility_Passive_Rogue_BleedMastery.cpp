// Source/DungeonForged/Private/GAS/Abilities/Passive/UDFAbility_Passive_Rogue_BleedMastery.cpp
#include "GAS/Abilities/Passive/UDFAbility_Passive_Rogue_BleedMastery.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Debuff_ArmorBreak.h"
#include "GAS/Effects/UGE_DoT_Bleed.h"
#include "GAS/Effects/UGE_Passive_BleedMastery_Extra.h"

UDFAbility_Passive_Rogue_BleedMastery::UDFAbility_Passive_Rogue_BleedMastery() = default;

void UDFAbility_Passive_Rogue_BleedMastery::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Passive_Rogue_BleedMastery);
	}
}

void UDFAbility_Passive_Rogue_BleedMastery::OnPassiveAbilityActivated(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo& ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	(void)Handle;
	(void)ActivationInfo;
	(void)TriggerEventData;
	if (!ActorInfo)
	{
		return;
	}
	if (UAbilityTask_WaitGameplayEvent* const W = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, FDFGameplayTags::Event_Passive_Rogue_BleedApplied, nullptr, false, true))
	{
		W->EventReceived.AddDynamic(this, &UDFAbility_Passive_Rogue_BleedMastery::OnRogueBleedApplied);
		W->ReadyForActivation();
	}
}

void UDFAbility_Passive_Rogue_BleedMastery::OnRogueBleedApplied(FGameplayEventData EventData)
{
	AActor* const Instigator = const_cast<AActor*>(EventData.Instigator.Get());
	AActor* const Victim = const_cast<AActor*>(EventData.Target.Get());
	if (!IsValid(Instigator) || !IsValid(Victim) || !Instigator->HasAuthority())
	{
		return;
	}
	if (GetAvatarActorFromActorInfo() != Instigator)
	{
		return;
	}
	UAbilitySystemComponent* const My = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* const VictimASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Victim);
	if (!My || !VictimASC)
	{
		return;
	}
	{
		FGameplayEffectContextHandle Ctx = My->MakeEffectContext();
		Ctx.AddInstigator(Instigator, Instigator);
		const FGameplayEffectSpecHandle Ex = My->MakeOutgoingSpec(UGE_Passive_BleedMastery_Extra::StaticClass(), 1.f, Ctx);
		if (Ex.IsValid() && Ex.Data)
		{
			My->ApplyGameplayEffectSpecToTarget(*Ex.Data, VictimASC);
		}
	}
	const int32 BleedCount = VictimASC->GetGameplayEffectCount(UGE_DoT_Bleed::StaticClass(), nullptr, true);
	if (BleedCount >= 2)
	{
		FGameplayEffectContextHandle AbCtx = My->MakeEffectContext();
		AbCtx.AddInstigator(Instigator, Instigator);
		FGameplayEffectSpecHandle Ab = My->MakeOutgoingSpec(UGE_Debuff_ArmorBreak::StaticClass(), 1.f, AbCtx);
		Ab = UAbilitySystemBlueprintLibrary::SetDuration(Ab, 3.f);
		if (Ab.IsValid() && Ab.Data && FDFGameplayTags::Data_Magnitude.IsValid())
		{
			Ab.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Magnitude, FMath::Max(0.01f, ArmorBreakMagnitude));
			My->ApplyGameplayEffectSpecToTarget(*Ab.Data, VictimASC);
		}
	}
}
