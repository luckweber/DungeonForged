// Source/DungeonForged/Private/GAS/Cues/UDFGameplayCueNotify_EnemyDeath.cpp
#include "GAS/Cues/UDFGameplayCueNotify_EnemyDeath.h"

#include "Animation/DFDeathAnimation.h"
#include "Characters/ADFEnemyBase.h"
#include "Combat/UDFDeathCinematicTypes.h"
#include "Combat/UDFDeathCinematicSubsystem.h"
#include "DungeonForgedModule.h"
#include "FX/UDFCameraShakeFunctionLibrary.h"
#include "FX/UDFHitStopSubsystem.h"
#include "GAS/DFGameplayTags.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

UDFGameplayCueNotify_EnemyDeath::UDFGameplayCueNotify_EnemyDeath()
{
	if (FDFGameplayTags::GameplayCue_Enemy_Death.IsValid())
	{
		GameplayCueTag = FDFGameplayTags::GameplayCue_Enemy_Death;
	}
}

bool UDFGameplayCueNotify_EnemyDeath::OnExecute_Implementation(
	AActor* const Target, const FGameplayCueParameters& Parameters) const
{
	if (!Target || Target->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}
	ADFEnemyBase* const Enemy = Cast<ADFEnemyBase>(Target);
	if (!Enemy)
	{
		return false;
	}

	UWorld* const W = Target->GetWorld();
	if (!W)
	{
		return false;
	}

	const FVector SpawnLoc = Parameters.Location.IsZero()
		? Target->GetActorLocation()
		: FVector(Parameters.Location);

	if (DeathBurstNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			W,
			DeathBurstNiagara,
			SpawnLoc,
			FRotator::ZeroRotator,
			FVector(1.f),
			true,
			true,
			ENCPoolMethod::AutoRelease);
	}
	if (DeathImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(W, DeathImpactSound, SpawnLoc);
	}

	AActor* const Killer = Parameters.Instigator.Get();
	FDFDeathCinematicContext Ctx;
	Ctx.Victim = Target;
	Ctx.Killer = Killer;
	Ctx.LethalImpactLocation = SpawnLoc;
	Ctx.ExperienceReward = Enemy->GetCachedExperienceReward();
	if (UDFDeathCinematicSubsystem* const DeathFx = W->GetSubsystem<UDFDeathCinematicSubsystem>())
	{
		DeathFx->PlayEnemyKillCinematic(Ctx);
	}

	DFDeathAnimation::LogEnemyDeath(
		2, Enemy,
		FString::Printf(TEXT("GameplayCue.Enemy.Death OnExecute Killer=%s"), *GetNameSafe(Killer)));
	UE_LOG(LogDFDeath, Verbose, TEXT("[Death] EnemyDeathCue %s Killer=%s"), *GetNameSafe(Target), *GetNameSafe(Killer));
	return true;
}
