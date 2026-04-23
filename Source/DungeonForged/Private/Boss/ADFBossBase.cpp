// Source/DungeonForged/Private/Boss/ADFBossBase.cpp
#include "Boss/ADFBossBase.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UDFGameplayEffectLibrary.h"
#include "GAS/Effects/UGE_BossEnrage.h"
#include "GAS/Effects/UGE_BossPhaseStats.h"
#include "GAS/Effects/UGE_Debuff_Stun.h"
#include "GAS/UDFGameplayAbility.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "NiagaraFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Kismet/GameplayStatics.h"

ADFBossBase::ADFBossBase()
{
	BossDisplayName = NSLOCTEXT("Boss", "DefaultName", "Boss");
	StunForPhaseTransition = UGE_Debuff_Stun::StaticClass();
	PhaseStatEffect = UGE_BossPhaseStats::StaticClass();
	EnrageEffect = UGE_BossEnrage::StaticClass();
}

void ADFBossBase::BeginPlay()
{
	Super::BeginPlay();
	LocalPhaseCache = CurrentPhase;
	bLocalEnragedCache = bIsEnraged;
	if (HasAuthority() && EnrageTimer > 0.f && !bIsEnraged)
	{
		GetWorldTimerManager().SetTimer(EnrageTimerHandle, this, &ADFBossBase::OnEnrageTimerExpired, EnrageTimer, false);
		bEnrageTimerSet = true;
	}
}

void ADFBossBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(EnrageTimerHandle);
	}
	ClearSpawnedMinions();
	Super::EndPlay(EndPlayReason);
}

void ADFBossBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADFBossBase, CurrentPhase);
	DOREPLIFETIME(ADFBossBase, bIsEnraged);
}

void ADFBossBase::NotifyHealthChangedFromAttributes(const float Current, const float Max)
{
	if (!HasAuthority() || bHasDied || Max <= 0.f)
	{
		return;
	}
	if (bIsEnraged)
	{
		return;
	}
	TryAdvancePhaseFromHealth(Current, Max);
}

void ADFBossBase::TryAdvancePhaseFromHealth(const float Current, const float Max)
{
	const float R = Current / Max;
	for (int32 TargetPhase = CurrentPhase + 1; TargetPhase <= MaxPhases; ++TargetPhase)
	{
		const int32 TI = TargetPhase - 2;
		if (!PhaseThresholds.IsValidIndex(TI))
		{
			break;
		}
		if (R <= PhaseThresholds[TI])
		{
			TriggerPhaseTransition(TargetPhase);
			return;
		}
	}
}

void ADFBossBase::TriggerPhaseTransition(const int32 NewPhase)
{
	if (!HasAuthority())
	{
		return;
	}
	if (NewPhase <= CurrentPhase || NewPhase > MaxPhases)
	{
		return;
	}
	const int32 Old = CurrentPhase;
	CurrentPhase = NewPhase;

	if (StunForPhaseTransition && AbilitySystemComponent)
	{
		FGameplayEffectContextHandle Ctx = AbilitySystemComponent->MakeEffectContext();
		Ctx.AddSourceObject(this);
		const FGameplayEffectSpecHandle H = AbilitySystemComponent->MakeOutgoingSpec(StunForPhaseTransition, 1.f, Ctx);
		if (FGameplayEffectSpec* const Spec = H.Data.Get())
		{
			const FGameplayTag D = FDFGameplayTags::Data_Duration.IsValid()
				? FDFGameplayTags::Data_Duration
				: FGameplayTag::RequestGameplayTag(FName("Data.Duration"), false);
			if (D.IsValid())
			{
				Spec->SetSetByCallerMagnitude(D, PhaseStunDuration);
			}
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec);
		}
	}

	if (PhaseStatEffect)
	{
		UDFGameplayEffectLibrary::ApplyEffectToSelf(this, PhaseStatEffect, 1.f);
	}

	const int32 AbIdx = NewPhase - 2;
	if (PhaseAbilities.IsValidIndex(AbIdx) && PhaseAbilities[AbIdx] && AbilitySystemComponent)
	{
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(PhaseAbilities[AbIdx], 1, INDEX_NONE, this));
	}

	Multicast_OnPhaseTransitionVFX();
	OnBossPhaseChanged.Broadcast(Old, NewPhase, this);
	ForceNetUpdate();
}

void ADFBossBase::OnEnrageTimerExpired()
{
	if (!HasAuthority() || bIsEnraged)
	{
		return;
	}
	if (EnrageEffect)
	{
		UDFGameplayEffectLibrary::ApplyEffectToSelf(this, EnrageEffect, 1.f);
	}
	bIsEnraged = true;
	OnRep_BossEnraged();
	Multicast_OnEnrageVFX();
	OnBossEnraged.Broadcast(this, true);
	ForceNetUpdate();
}

void ADFBossBase::OnRep_BossCurrentPhase()
{
	if (LocalPhaseCache != CurrentPhase)
	{
		const int32 Old = LocalPhaseCache;
		LocalPhaseCache = CurrentPhase;
		OnBossPhaseChanged.Broadcast(Old, CurrentPhase, this);
	}
}

void ADFBossBase::OnRep_BossEnraged()
{
	if (bIsEnraged != bLocalEnragedCache)
	{
		bLocalEnragedCache = bIsEnraged;
		OnBossEnraged.Broadcast(this, bIsEnraged);
	}
}

void ADFBossBase::Multicast_OnPhaseTransitionVFX_Implementation()
{
	if (USkeletalMeshComponent* const Sk = GetMesh())
	{
		if (UAnimInstance* const AI = Sk->GetAnimInstance())
		{
			if (PhaseTransitionMontage)
			{
				AI->Montage_Play(PhaseTransitionMontage, 1.f, EMontagePlayReturnType::Duration, 0.f, true);
			}
		}
	}
	if (PhaseTransitionVFX)
	{
		const FVector L = GetActorLocation();
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, PhaseTransitionVFX, L, FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::None, true);
	}
}

void ADFBossBase::Multicast_OnEnrageVFX_Implementation()
{
	if (USkeletalMeshComponent* const Sk = GetMesh())
	{
		if (UAnimInstance* const AI = Sk->GetAnimInstance())
		{
			if (EnrageRoarMontage)
			{
				AI->Montage_Play(EnrageRoarMontage, 1.f, EMontagePlayReturnType::Duration, 0.f, true);
			}
		}
	}
	if (EnrageVFX)
	{
		const FVector L = GetActorLocation() + FVector(0.f, 0.f, 80.f);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, EnrageVFX, L, FRotator::ZeroRotator, FVector(1.2f), true, true, ENCPoolMethod::None, true);
	}
}

void ADFBossBase::Multicast_BossLocalAttackFX_Implementation(
	const FVector& Origin,
	const TSubclassOf<UCameraShakeBase> CameraShake,
	UNiagaraSystem* const Niagara,
	const float CameraShakeInnerRadius,
	const float CameraShakeOuterRadius)
{
	if (IsValid(Niagara))
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, Niagara, Origin, FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::None, true);
	}
	if (CameraShake)
	{
		UGameplayStatics::PlayWorldCameraShake(
			this,
			CameraShake,
			Origin,
			CameraShakeInnerRadius,
			CameraShakeOuterRadius,
			1.f,
			false);
	}
}

void ADFBossBase::Multicast_PlayLocalMontage_Implementation(
	UAnimMontage* const Montage,
	const float PlayRate,
	const FName StartSectionName)
{
	if (!Montage)
	{
		return;
	}
	if (USkeletalMeshComponent* const Sk = GetMesh())
	{
		if (UAnimInstance* const AI = Sk->GetAnimInstance())
		{
			AI->Montage_Play(Montage, PlayRate, EMontagePlayReturnType::Duration, 0.f, true);
			if (StartSectionName != NAME_None)
			{
				AI->Montage_JumpToSection(StartSectionName, Montage);
			}
		}
	}
}

void ADFBossBase::HandleServerDeath(AActor* Killer)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(EnrageTimerHandle);
	}
	ClearSpawnedMinions();
	Super::HandleServerDeath(Killer);
}

void ADFBossBase::RegisterSpawnedMinion(ADFEnemyBase* Minion)
{
	if (!Minion)
	{
		return;
	}
	SpawnedMinions.Add(Minion);
	Minion->OnEnemyDied.AddUniqueDynamic(this, &ADFBossBase::HandleMinionEnemyDied);
	if (UAbilitySystemComponent* const ASC = Minion->GetAbilitySystemComponent())
	{
		ASC->AddLooseGameplayTag(FDFGameplayTags::State_Spawned_Boss);
	}
}

int32 ADFBossBase::GetLivingMinionCount() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<ADFEnemyBase>& M : SpawnedMinions)
	{
		if (M.IsValid())
		{
			++N;
		}
	}
	return N;
}

void ADFBossBase::HandleMinionEnemyDied(AActor* const Enemy, AActor* const Killer, const float Exp)
{
	(void)Killer;
	(void)Exp;
	SpawnedMinions.RemoveAll([Enemy](const TWeakObjectPtr<ADFEnemyBase>& W) { return W.Get() == Enemy; });
}

void ADFBossBase::ClearSpawnedMinions()
{
	for (TWeakObjectPtr<ADFEnemyBase>& M : SpawnedMinions)
	{
		if (ADFEnemyBase* const A = M.Get())
		{
			A->OnEnemyDied.RemoveDynamic(this, &ADFBossBase::HandleMinionEnemyDied);
			if (A->GetAbilitySystemComponent())
			{
				A->GetAbilitySystemComponent()->RemoveLooseGameplayTag(FDFGameplayTags::State_Spawned_Boss);
			}
			if (HasAuthority() && A->GetWorld())
			{
				A->Destroy();
			}
		}
	}
	SpawnedMinions.Empty();
}
