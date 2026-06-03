#include "GAS/Abilities/Universal/UDFAbility_Universal_CallLightning.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/ADFEnemyBase.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"
#include "FX/UDFCombatFeedbackLibrary.h"
#include "GameFramework/Character.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Cooldown_Universal_CallLightning.h"
#include "GAS/Effects/UGE_Damage_Magic.h"
#include "GAS/UDFAttributeSet.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

UDFAbility_Universal_CallLightning::UDFAbility_Universal_CallLightning()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	AbilityCost_Mana = 50.f;
	BaseCooldown = 45.f;
	LightningDamageEffectClass = UGE_Damage_Magic::StaticClass();
	CooldownGameplayEffectClass = UGE_Cooldown_Universal_CallLightning::StaticClass();
}

void UDFAbility_Universal_CallLightning::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Universal_CallLightning);
		ActivationOwnedTags.AddTag(FDFGameplayTags::State_Casting);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
	}
}

void UDFAbility_Universal_CallLightning::StrikeAtLocation(const FVector& Loc)
{
	ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* const SourceASC = GetAbilitySystemComponentFromActorInfo();
	UWorld* const World = GetWorld();
	if (!Char || !SourceASC || !World || !Char->HasAuthority() || !LightningDamageEffectClass)
	{
		return;
	}
	if (StrikeVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			Char, StrikeVFX, Loc, FRotator::ZeroRotator, FVector(1.2f), true, true, ENCPoolMethod::None, true);
	}
	const float Int = SourceASC->GetNumericAttribute(UDFAttributeSet::GetIntelligenceAttribute());
	const float Dmg = BonusBaseDamage + Int * 1.5f;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(CallLightning), false, Char);
	TArray<FOverlapResult> Hits;
	FCollisionObjectQueryParams Obj(ECC_Pawn);
	World->OverlapMultiByObjectType(
		Hits, Loc, FQuat::Identity, Obj, FCollisionShape::MakeSphere(StrikeRadius), Q);
	for (const FOverlapResult& R : Hits)
	{
		AActor* const Target = R.GetActor();
		if (!Target || Target == Char || !Cast<ADFEnemyBase>(Target))
		{
			continue;
		}
		UAbilitySystemComponent* const TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
		if (!TargetASC)
		{
			continue;
		}
		FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
		Ctx.AddInstigator(Char, Char);
		const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(LightningDamageEffectClass, 1.f, Ctx);
		if (!Spec.IsValid() || !Spec.Data.IsValid())
		{
			continue;
		}
		if (FDFGameplayTags::Data_Damage.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, Dmg);
		}
		if (FDFGameplayTags::Effect_Element_Lightning.IsValid())
		{
			Spec.Data->AddDynamicAssetTag(FDFGameplayTags::Effect_Element_Lightning);
		}
		UDFCombatFeedbackLibrary::MarkSpecCombatFeedbackCentralized(*Spec.Data.Get());
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}

void UDFAbility_Universal_CallLightning::ActivateAbility(
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
	if (ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		const FVector Loc = Char->GetActorLocation() + Char->GetActorForwardVector() * StrikeRange;
		StrikeAtLocation(Loc);
	}
	K2_CommitAbilityCooldown(true, true);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
