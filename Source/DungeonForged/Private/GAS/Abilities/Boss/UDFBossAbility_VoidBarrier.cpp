// Source/DungeonForged/Private/GAS/Abilities/Boss/UDFBossAbility_VoidBarrier.cpp
#include "GAS/Abilities/Boss/UDFBossAbility_VoidBarrier.h"
#include "GAS/Abilities/Boss/DFBossAbilityCommons.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Boss_VoidBarrier.h"
#include "GAS/Effects/UGE_Cooldown_Boss_VoidBarrier.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemComponent.h"
#include "Boss/ADFBossBase.h"
#include "Boss/ADFVoidOrbActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/Character.h"

UDFBossAbility_VoidBarrier::UDFBossAbility_VoidBarrier()
{
	bSourceObjectMustBeBoss = true;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseCooldown = 60.f;
	BarrierClass = UGE_Boss_VoidBarrier::StaticClass();
	CooldownClass = UGE_Cooldown_Boss_VoidBarrier::StaticClass();
	OrbClass = ADFVoidOrbActor::StaticClass();
}

void UDFBossAbility_VoidBarrier::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Boss_VoidBarrier);
	}
}

bool UDFBossAbility_VoidBarrier::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) || !ActorInfo)
	{
		return false;
	}
	if (const ADFBossBase* B = Cast<ADFBossBase>(ActorInfo->AvatarActor.Get()))
	{
		return B->CurrentPhase >= 2;
	}
	return false;
}

void UDFBossAbility_VoidBarrier::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	(void)TriggerEventData;
	if (!ActorInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo, nullptr))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	UAbilitySystemComponent* const ASC = ActorInfo->AbilitySystemComponent.Get();
	ADFBossBase* const Boss = Cast<ADFBossBase>(GetAvatarActorFromActorInfo());
	if (!ASC || !Boss || !GetWorld())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	DFBossAbilityCommons::ApplySetCallerCooldown(ASC, CooldownClass, 60.f);
	if (BarrierClass)
	{
		const FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
		const FGameplayEffectSpecHandle S = ASC->MakeOutgoingSpec(BarrierClass, 1.f, Ctx);
		if (S.IsValid() && S.Data.IsValid() && FDFGameplayTags::Data_Duration.IsValid())
		{
			S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, BarrierDuration);
			ActiveBarrier = ASC->ApplyGameplayEffectSpecToSelf(*S.Data);
		}
	}
	Orbs.Empty();
	if (!IsRunningDedicatedServer() && Boss->GetMesh() && BarrierVfx)
	{
		SpawnedBarrierNiagara = UNiagaraFunctionLibrary::SpawnSystemAttached(BarrierVfx, Boss->GetMesh(), NAME_None, FVector(0.f, 0.f, 80.f), FRotator::ZeroRotator, FVector::OneVector, EAttachLocation::SnapToTarget, true, ENCPoolMethod::None, true, true);
	}
	if (OrbClass && GetWorld())
	{
		FActorSpawnParameters Sp;
		Sp.Owner = Boss;
		Sp.Instigator = Boss;
		Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		const float R = OrbitRadius;
		const FVector Offsets[4] = {FVector(R, 0.f, 0.f), FVector(0.f, R, 0.f), FVector(-R, 0.f, 0.f), FVector(0.f, -R, 0.f)};
		for (int32 I = 0; I < 4; ++I)
		{
			if (ADFVoidOrbActor* O = GetWorld()->SpawnActor<ADFVoidOrbActor>(OrbClass, FTransform(Offsets[I]), Sp))
			{
				O->Init(this, Boss, Offsets[I]);
				Orbs.Add(O);
			}
		}
	}
	OrbsKilled = 0;
	bBarrierEnded = false;
	if (UAbilityTask_WaitDelay* D = UAbilityTask_WaitDelay::WaitDelay(this, BarrierDuration))
	{
		D->OnFinish.AddDynamic(this, &UDFBossAbility_VoidBarrier::OnDurationExpired);
		D->ReadyForActivation();
	}
}

void UDFBossAbility_VoidBarrier::OnDurationExpired()
{
	FinishBarrierFromTimeout();
}

void UDFBossAbility_VoidBarrier::FinishBarrierFromTimeout()
{
	if (bBarrierEnded)
	{
		return;
	}
	bBarrierEnded = true;
	ADFBossBase* const Boss = Cast<ADFBossBase>(GetAvatarActorFromActorInfo());
	if (UAbilitySystemComponent* const ASC = Boss ? Boss->GetAbilitySystemComponent() : nullptr)
	{
		if (ActiveBarrier.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(ActiveBarrier, 1);
		}
		ActiveBarrier = FActiveGameplayEffectHandle();
	}
	DestroyOrbsSilently();
	if (SpawnedBarrierNiagara)
	{
		SpawnedBarrierNiagara->DestroyComponent();
		SpawnedBarrierNiagara = nullptr;
	}
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UDFBossAbility_VoidBarrier::DestroyOrbsSilently()
{
	for (ADFVoidOrbActor* O : Orbs)
	{
		if (O)
		{
			O->Orphan();
			O->Destroy();
		}
	}
	Orbs.Empty();
}

void UDFBossAbility_VoidBarrier::NotifyVoidOrbDestroyed(ADFVoidOrbActor* const Orb)
{
	(void)Orb;
	if (bBarrierEnded)
	{
		return;
	}
	OrbsKilled++;
	if (OrbsKilled < 4)
	{
		return;
	}
	bBarrierEnded = true;
	ADFBossBase* const Boss = Cast<ADFBossBase>(GetAvatarActorFromActorInfo());
	if (UAbilitySystemComponent* const ASC = Boss ? Boss->GetAbilitySystemComponent() : nullptr)
	{
		if (ActiveBarrier.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(ActiveBarrier, 1);
		}
		ActiveBarrier = FActiveGameplayEffectHandle();
	}
	for (TObjectPtr<ADFVoidOrbActor>& P : Orbs)
	{
		if (P && P != Orb)
		{
			P->Orphan();
			P->Destroy();
		}
	}
	Orbs.Empty();
	if (SpawnedBarrierNiagara)
	{
		SpawnedBarrierNiagara->DestroyComponent();
		SpawnedBarrierNiagara = nullptr;
	}
	if (Boss)
	{
		if (UAbilitySystemComponent* const A = Boss->GetAbilitySystemComponent())
		{
			DFBossAbilityCommons::StunTarget(A, Boss, BossStunOnBreakSeconds);
		}
	}
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
