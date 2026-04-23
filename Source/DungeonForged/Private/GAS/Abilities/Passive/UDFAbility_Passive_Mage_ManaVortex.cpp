// Source/DungeonForged/Private/GAS/Abilities/Passive/UDFAbility_Passive_Mage_ManaVortex.cpp
#include "GAS/Abilities/Passive/UDFAbility_Passive_Mage_ManaVortex.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Passive_Mage_ManaVortex_Refund.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

UDFAbility_Passive_Mage_ManaVortex::UDFAbility_Passive_Mage_ManaVortex() = default;

void UDFAbility_Passive_Mage_ManaVortex::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Passive_Mage_ManaVortex);
	}
}

void UDFAbility_Passive_Mage_ManaVortex::OnPassiveAbilityActivated(
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
			this, FDFGameplayTags::Event_Ability_Kill, nullptr, false, true))
	{
		W->EventReceived.AddDynamic(this, &UDFAbility_Passive_Mage_ManaVortex::OnAbilityKillEvent);
		W->ReadyForActivation();
	}
}

void UDFAbility_Passive_Mage_ManaVortex::OnAbilityKillEvent(FGameplayEventData /*EventData*/)
{
	AActor* const Av = GetAvatarActorFromActorInfo();
	if (!Av || !Av->HasAuthority())
	{
		return;
	}
	UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}
	const FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
	const FGameplayEffectSpecHandle S = ASC->MakeOutgoingSpec(UGE_Passive_Mage_ManaVortex_Refund::StaticClass(), 1.f, Ctx);
	if (S.IsValid() && S.Data)
	{
		ASC->ApplyGameplayEffectSpecToSelf(*S.Data);
	}
	if (ManaVortexVFX)
	{
		const FVector L = Av->GetActorLocation() + FVector(0.f, 0.f, 40.f);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, ManaVortexVFX, L, FRotator::ZeroRotator, FVector(0.5f, 0.5f, 0.5f), true, true, ENCPoolMethod::AutoRelease, true);
	}
}
