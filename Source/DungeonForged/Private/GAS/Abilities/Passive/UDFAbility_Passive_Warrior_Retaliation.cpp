// Source/DungeonForged/Private/GAS/Abilities/Passive/UDFAbility_Passive_Warrior_Retaliation.cpp
#include "GAS/Abilities/Passive/UDFAbility_Passive_Warrior_Retaliation.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/DFRogueGAS.h"
#include "GAS/Effects/UGE_Damage_Physical.h"
#include "GAS/UDFAttributeSet.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

UDFAbility_Passive_Warrior_Retaliation::UDFAbility_Passive_Warrior_Retaliation() = default;

void UDFAbility_Passive_Warrior_Retaliation::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Passive_Warrior_Retaliation);
	}
}

void UDFAbility_Passive_Warrior_Retaliation::OnPassiveAbilityActivated(
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
			this, FDFGameplayTags::Event_Hit_Received, nullptr, false, true))
	{
		W->EventReceived.AddDynamic(this, &UDFAbility_Passive_Warrior_Retaliation::OnHitReceived);
		W->ReadyForActivation();
	}
}

void UDFAbility_Passive_Warrior_Retaliation::OnHitReceived(FGameplayEventData EventData)
{
	AActor* const VictimAv = GetAvatarActorFromActorInfo();
	if (!VictimAv || !VictimAv->HasAuthority())
	{
		return;
	}
	UAbilitySystemComponent* const SelfASC = GetAbilitySystemComponentFromActorInfo();
	if (!SelfASC)
	{
		return;
	}
	const float MaxH = SelfASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
	if (MaxH < KINDA_SMALL_NUMBER)
	{
		return;
	}
	if (EventData.EventMagnitude <= 0.05f * MaxH)
	{
		return;
	}
	AActor* const Attacker = const_cast<AActor*>(EventData.Instigator.Get());
	if (!IsValid(Attacker))
	{
		return;
	}
	UAbilitySystemComponent* const TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Attacker);
	if (!TargetASC)
	{
		return;
	}
	const float Str = SelfASC->GetNumericAttribute(UDFAttributeSet::GetStrengthAttribute());
	const float Sbc = DF_Rogue_CompensatePhysicalSetBy(SelfASC, Str * 0.3f);
	const FGameplayEffectContextHandle Ctx = DF_Rogue_EffectContext(SelfASC, VictimAv, nullptr);
	const FGameplayEffectSpecHandle S = SelfASC->MakeOutgoingSpec(UGE_Damage_Physical::StaticClass(), 1.f, Ctx);
	if (S.IsValid() && S.Data && FDFGameplayTags::Data_Damage.IsValid())
	{
		S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, Sbc);
		SelfASC->ApplyGameplayEffectSpecToTarget(*S.Data, TargetASC);
	}
	if (RetaliationRiposteVFX)
	{
		const FVector L = Attacker->GetActorLocation() + FVector(0.f, 0.f, 40.f);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, RetaliationRiposteVFX, L, FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::AutoRelease, true);
	}
}
