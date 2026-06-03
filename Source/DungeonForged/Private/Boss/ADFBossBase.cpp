// Source/DungeonForged/Private/Boss/ADFBossBase.cpp
#include "Boss/ADFBossBase.h"
#include "Boss/ADFMeteorWarningDecal.h"
#include "Boss/UDFBossMinionComponent.h"
#include "Combat/UDFCombatDirectorSubsystem.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UDFGameplayEffectLibrary.h"
#include "GAS/UDFAttributeSet.h"
#include "GAS/Effects/UGE_BossEnrage.h"
#include "GAS/Effects/UGE_BossPhaseStats.h"
#include "GAS/Effects/UGE_Debuff_Stun.h"
#include "GAS/UDFGameplayAbility.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpec.h"
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
	// All clients: boss phases, debuffs, telegraphs; overrides enemy `Minimal` at construction.
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);
	}
}

void ADFBossBase::BeginPlay()
{
	Super::BeginPlay();
	LocalPhaseCache = CurrentPhase;
	bLocalEnragedCache = bIsEnraged;
	if (HasAuthority() && EnrageTimer > 0.f && !bIsEnraged)
	{
		EnrageCountdownEndWorldTime = GetWorld()->GetTimeSeconds() + EnrageTimer;
		GetWorldTimerManager().SetTimer(EnrageTimerHandle, this, &ADFBossBase::OnEnrageTimerExpired, EnrageTimer, false);
		bEnrageTimerSet = true;
		ForceNetUpdate();
	}
	if (HasAuthority() && PhaseSlamAbility && AbilitySystemComponent)
	{
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(PhaseSlamAbility, 1, INDEX_NONE, this));
	}
}

void ADFBossBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(EnrageTimerHandle);
		GetWorldTimerManager().ClearTimer(VulnerabilityTimerHandle);
	}
	ClearSpawnedMinions();
	Super::EndPlay(EndPlayReason);
}

void ADFBossBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADFBossBase, CurrentPhase);
	DOREPLIFETIME(ADFBossBase, bIsEnraged);
	DOREPLIFETIME(ADFBossBase, EnrageCountdownEndWorldTime);
	DOREPLIFETIME(ADFBossBase, BossDisplayName);
}

void ADFBossBase::NotifyHealthChangedFromAttributes(const float Current, const float Max)
{
	if (!HasAuthority() || bHasDied || Max <= 0.f)
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

	if (HasAuthority() && PhaseSlamAbility && AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbilityByClass(PhaseSlamAbility, true);
	}
	else if (StunForPhaseTransition && AbilitySystemComponent)
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
	ApplyPhaseCooldownProfile(NewPhase);

	if (bClearMinionsOnPhaseChange)
	{
		ClearSpawnedMinions();
	}

	const int32 AbIdx = NewPhase - 2;
	if (PhaseAbilities.IsValidIndex(AbIdx) && PhaseAbilities[AbIdx] && AbilitySystemComponent)
	{
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(PhaseAbilities[AbIdx], 1, INDEX_NONE, this));
	}

	Multicast_OnPhaseTransitionVFX();
	BeginBossVulnerabilityWindow();
	OnBossPhaseChanged.Broadcast(Old, NewPhase, this);
	ForceNetUpdate();
}

void ADFBossBase::BeginBossVulnerabilityWindow()
{
	if (!HasAuthority() || !AbilitySystemComponent || !FDFGameplayTags::State_BossVulnerable.IsValid())
	{
		return;
	}
	AbilitySystemComponent->AddLooseGameplayTag(FDFGameplayTags::State_BossVulnerable, 1);
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(VulnerabilityTimerHandle);
		const float Duration = FMath::Max(0.1f, BossVulnerabilityDuration);
		World->GetTimerManager().SetTimer(
			VulnerabilityTimerHandle, this, &ADFBossBase::EndBossVulnerabilityWindow, Duration, false);
	}
}

void ADFBossBase::EndBossVulnerabilityWindow()
{
	if (AbilitySystemComponent && FDFGameplayTags::State_BossVulnerable.IsValid())
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(FDFGameplayTags::State_BossVulnerable, 1);
	}
}

void ADFBossBase::ApplyPhaseCooldownProfile(const int32 NewPhase)
{
	if (!AbilitySystemComponent)
	{
		return;
	}
	const int32 Idx = NewPhase - 1;
	if (!PhaseCooldownReduction.IsValidIndex(Idx))
	{
		return;
	}
	const float TargetCdr = FMath::Clamp(PhaseCooldownReduction[Idx], 0.f, 0.5f);
	if (UDFAttributeSet* const Stats = const_cast<UDFAttributeSet*>(AbilitySystemComponent->GetSet<UDFAttributeSet>()))
	{
		Stats->SetCooldownReduction(TargetCdr);
	}
}


void ADFBossBase::OnEnrageTimerExpired()
{
	if (!HasAuthority() || bIsEnraged)
	{
		return;
	}
	EnrageCountdownEndWorldTime = 0.f;
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
	if (bIsEnraged)
	{
		EnrageCountdownEndWorldTime = 0.f;
	}
}

float ADFBossBase::GetEnrageSecondsRemaining() const
{
	if (bIsEnraged || EnrageCountdownEndWorldTime <= 0.f)
	{
		return 0.f;
	}
	const UWorld* const World = GetWorld();
	if (!World)
	{
		return 0.f;
	}
	return FMath::Max(0.f, EnrageCountdownEndWorldTime - World->GetTimeSeconds());
}

void ADFBossBase::OnRep_EnrageCountdownEndWorldTime()
{
	// Clients refresh HUD via widget timer/delegates.
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

void ADFBossBase::Multicast_PlayMeteorWarning_Implementation(
	const FVector& Location,
	const float DecalRadius,
	const float DurationSeconds,
	const TSubclassOf<ADFMeteorWarningDecal> DecalClass)
{
	if (!DecalClass || IsRunningDedicatedServer())
	{
		return;
	}
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}
	FActorSpawnParameters Sp;
	Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Sp.Owner = this;
	Sp.Instigator = this;
	if (ADFMeteorWarningDecal* const Decal = World->SpawnActor<ADFMeteorWarningDecal>(
		DecalClass, FTransform(FRotator(-90.f, 0.f, 0.f), Location), Sp))
	{
		Decal->ConfigureWarning(DecalRadius, DurationSeconds);
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
		GetWorldTimerManager().ClearTimer(VulnerabilityTimerHandle);
	}
	EndBossVulnerabilityWindow();
	ClearSpawnedMinions();
	Super::HandleServerDeath(Killer);
}

void ADFBossBase::RegisterSpawnedMinion(ADFEnemyBase* const Minion, const EDFBossMinionRole MinionRole)
{
	if (!Minion)
	{
		return;
	}
	if (UWorld* const World = GetWorld())
	{
		if (UDFCombatDirectorSubsystem* const Director = World->GetSubsystem<UDFCombatDirectorSubsystem>())
		{
			Director->RegisterBossMinion();
		}
	}
	UDFBossMinionComponent* MinionComp = Minion->FindComponentByClass<UDFBossMinionComponent>();
	if (!MinionComp)
	{
		MinionComp = NewObject<UDFBossMinionComponent>(Minion, UDFBossMinionComponent::StaticClass());
		if (MinionComp)
		{
			MinionComp->RegisterComponent();
		}
	}
	if (MinionComp)
	{
		MinionComp->InitializeMinion(this, MinionRole);
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
	if (UWorld* const World = GetWorld())
	{
		if (UDFCombatDirectorSubsystem* const Director = World->GetSubsystem<UDFCombatDirectorSubsystem>())
		{
			Director->UnregisterBossMinion();
		}
	}
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
				if (FDFGameplayTags::State_Spawned_Boss_Guard.IsValid())
				{
					A->GetAbilitySystemComponent()->RemoveLooseGameplayTag(FDFGameplayTags::State_Spawned_Boss_Guard);
				}
				if (FDFGameplayTags::State_Spawned_Boss_Exploder.IsValid())
				{
					A->GetAbilitySystemComponent()->RemoveLooseGameplayTag(FDFGameplayTags::State_Spawned_Boss_Exploder);
				}
			}
			if (HasAuthority() && A->GetWorld())
			{
				if (UDFCombatDirectorSubsystem* const Director = A->GetWorld()->GetSubsystem<UDFCombatDirectorSubsystem>())
				{
					Director->UnregisterBossMinion();
				}
				A->Destroy();
			}
		}
	}
	SpawnedMinions.Empty();
}
