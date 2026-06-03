#include "GAS/Abilities/Universal/UDFAbility_Universal_Siphon.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/ADFEnemyBase.h"
#include "CollisionQueryParams.h"
#include "FX/UDFCombatFeedbackLibrary.h"
#include "GameFramework/Character.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Cooldown_Universal_Siphon.h"
#include "GAS/Effects/UGE_Damage_True.h"
#include "GAS/Elemental/UDFElementalLibrary.h"
#include "GAS/UDFAttributeSet.h"
#include "WorldCollision.h"

UDFAbility_Universal_Siphon::UDFAbility_Universal_Siphon()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	AbilityCost_Mana = 35.f;
	BaseCooldown = 25.f;
	TrueDamageEffectClass = UGE_Damage_True::StaticClass();
	CooldownGameplayEffectClass = UGE_Cooldown_Universal_Siphon::StaticClass();
}

void UDFAbility_Universal_Siphon::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Universal_Siphon);
		ActivationOwnedTags.AddTag(FDFGameplayTags::State_Attacking);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
	}
}

void UDFAbility_Universal_Siphon::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	(void)TriggerEventData;
	if (!ActorInfo)
	{
		EndAbility(Handle, nullptr, ActivationInfo, true, true);
		return;
	}
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo, nullptr))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	const bool bHasMontage = AbilityMontage != nullptr;
	if (FDFGameplayTags::Event_Universal_Siphon_Trace.IsValid() && bHasMontage)
	{
		if (UAbilityTask_WaitGameplayEvent* const Wait = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, FDFGameplayTags::Event_Universal_Siphon_Trace, nullptr, false, false))
		{
			Wait->EventReceived.AddDynamic(this, &UDFAbility_Universal_Siphon::OnSiphonTraceEvent);
			Wait->ReadyForActivation();
		}
		if (UAbilityTask_WaitDelay* const Fallback = UAbilityTask_WaitDelay::WaitDelay(this, 1.25f))
		{
			Fallback->OnFinish.AddDynamic(this, &UDFAbility_Universal_Siphon::OnSiphonFallbackDelay);
			Fallback->ReadyForActivation();
		}
		PlayAbilityMontage();
	}
	else
	{
		ApplySiphonHit();
		K2_CommitAbilityCooldown(true, true);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UDFAbility_Universal_Siphon::OnSiphonTraceEvent(FGameplayEventData Payload)
{
	(void)Payload;
	FinishSiphon();
}

void UDFAbility_Universal_Siphon::OnSiphonFallbackDelay()
{
	FinishSiphon();
}

void UDFAbility_Universal_Siphon::FinishSiphon()
{
	if (bSiphonResolved)
	{
		return;
	}
	bSiphonResolved = true;
	ApplySiphonHit();
	K2_CommitAbilityCooldown(true, true);
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UDFAbility_Universal_Siphon::ApplySiphonHit()
{
	ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* const SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!Char || !SourceASC || !Char->HasAuthority())
	{
		return;
	}
	const FVector Start = Char->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector End = Start + Char->GetActorForwardVector() * TraceRange;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(Siphon), false, Char);
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Q))
	{
		return;
	}
	AActor* const Target = Hit.GetActor();
	if (!Target || !Cast<ADFEnemyBase>(Target))
	{
		return;
	}
	UAbilitySystemComponent* const TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC || !TrueDamageEffectClass)
	{
		return;
	}
	const float Int = SourceASC->GetNumericAttribute(UDFAttributeSet::GetIntelligenceAttribute());
	const float Dmg = BonusBaseDamage + Int * 1.2f;
	FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
	Ctx.AddInstigator(Char, Char);
	Ctx.AddHitResult(Hit);
	const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(TrueDamageEffectClass, 1.f, Ctx);
	if (!Spec.IsValid() || !Spec.Data.IsValid())
	{
		return;
	}
	if (FDFGameplayTags::Data_Damage.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, Dmg);
	}
	if (FDFGameplayTags::Effect_Element_Arcane.IsValid())
	{
		Spec.Data->AddDynamicAssetTag(FDFGameplayTags::Effect_Element_Arcane);
	}
	UDFCombatFeedbackLibrary::MarkSpecCombatFeedbackCentralized(*Spec.Data.Get());
	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	SourceASC->ApplyModToAttribute(UDFAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, Dmg);
}
