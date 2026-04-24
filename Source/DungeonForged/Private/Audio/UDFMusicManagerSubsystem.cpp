// Source/DungeonForged/Private/Audio/UDFMusicManagerSubsystem.cpp
#include "Audio/UDFMusicManagerSubsystem.h"
#include "Audio/ADFMusicLayerHost.h"
#include "GAS/DFGameplayTags.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Components/AudioComponent.h"
#include "AbilitySystemComponent.h"
#include "Sound/SoundBase.h"
#include "GameplayTagContainer.h"

void UDFMusicManagerSubsystem::Initialize(FSubsystemCollectionBase& /*Collection*/)
{
	if (!ShouldRunMusic() || !GetWorld())
	{
		return;
	}
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	P.bDeferConstruction = false;
	ADFMusicLayerHost* const H = GetWorld()->SpawnActor<ADFMusicLayerHost>(ADFMusicLayerHost::StaticClass(), FTransform::Identity, P);
	Host = H;
	ApplyInitialExploration();
}

void UDFMusicManagerSubsystem::Deinitialize()
{
	UnregisterLocalPlayerForCombatMusic();
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(CrossfadeLerpHandle);
		W->GetTimerManager().ClearTimer(CombatExploreTimer);
		W->GetTimerManager().ClearTimer(VictoryReturnTimer);
	}
	if (ADFMusicLayerHost* H = Host.Get())
	{
		H->Destroy();
	}
	Host = nullptr;
}

bool UDFMusicManagerSubsystem::ShouldRunMusic() const
{
	if (UWorld* const W = GetWorld())
	{
		return W->GetNetMode() != NM_DedicatedServer;
	}
	return true;
}

void UDFMusicManagerSubsystem::RegisterLocalPlayerForCombatMusic(
	UAbilitySystemComponent* const PlayerASC,
	AActor* const /*OwnerForCleanup*/)
{
	if (!PlayerASC || !FDFGameplayTags::State_InCombat.IsValid())
	{
		return;
	}
	UnregisterLocalPlayerForCombatMusic();
	BoundASC = PlayerASC;
	CombatTagHandle = PlayerASC
	                    ->RegisterGameplayTagEvent(
							FDFGameplayTags::State_InCombat,
							EGameplayTagEventType::NewOrRemoved)
	                    .AddUObject(this, &UDFMusicManagerSubsystem::OnInCombatTagChanged);
}

void UDFMusicManagerSubsystem::UnregisterLocalPlayerForCombatMusic()
{
	if (UAbilitySystemComponent* const A = BoundASC.Get())
	{
		if (CombatTagHandle.IsValid() && FDFGameplayTags::State_InCombat.IsValid())
		{
			A->UnregisterGameplayTagEvent(
				CombatTagHandle,
				FDFGameplayTags::State_InCombat,
				EGameplayTagEventType::NewOrRemoved);
		}
	}
	CombatTagHandle = FDelegateHandle();
	BoundASC = nullptr;
}

void UDFMusicManagerSubsystem::OnInCombatTagChanged(
	const FGameplayTag /*Tag*/,
	int32 const NewCount)
{
	if (!ShouldRunMusic() || !GetWorld())
	{
		return;
	}
	if (NewCount > 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(CombatExploreTimer);
		SetMusicState(EMusicState::Combat);
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(
			CombatExploreTimer,
			this,
			&UDFMusicManagerSubsystem::OnCombatClearedToExploration,
			5.f,
			false);
	}
}

void UDFMusicManagerSubsystem::OnCombatClearedToExploration()
{
	SetMusicState(EMusicState::Exploration);
}

void UDFMusicManagerSubsystem::OnBossEncounterStarted()
{
	SetMusicState(EMusicState::Boss);
}

void UDFMusicManagerSubsystem::OnBossDefeated()
{
	SetMusicState(EMusicState::Victory);
}

void UDFMusicManagerSubsystem::OnVictoryReturnToExploration()
{
	VictoryReturnTimer.Invalidate();
	if (!ShouldRunMusic() || !GetWorld())
	{
		return;
	}
	SetMusicState(EMusicState::Exploration);
}

void UDFMusicManagerSubsystem::ApplyInitialExploration()
{
	ADFMusicLayerHost* const H = Host.Get();
	if (!H)
	{
		return;
	}
	AssignLoopingSoundIfNeeded(H->MusicLayerBase, SoundExplorationBase, true);
	AssignLoopingSoundIfNeeded(H->MusicLayerCombat, SoundCombat, false);
	AssignLoopingSoundIfNeeded(H->MusicLayerBoss, SoundBoss, false);
	SetLayerTargetVolumes(1.f, 0.f, 0.f);
	CBase = TBase;
	CCombat = TCombat;
	CBoss = TBoss;
	if (H->MusicLayerBase)
	{
		H->MusicLayerBase->SetVolumeMultiplier(CBase);
	}
	if (H->MusicLayerCombat)
	{
		H->MusicLayerCombat->SetVolumeMultiplier(CCombat);
	}
	if (H->MusicLayerBoss)
	{
		H->MusicLayerBoss->SetVolumeMultiplier(CBoss);
	}
	CurrentState = EMusicState::Exploration;
}

void UDFMusicManagerSubsystem::AssignLoopingSoundIfNeeded(
	UAudioComponent* C,
	USoundBase* NewSound,
	const bool bShouldBePlaying) const
{
	if (!C)
	{
		return;
	}
	if (NewSound)
	{
		if (C->GetSound() != NewSound)
		{
			C->SetSound(NewSound);
		}
	}
	if (bShouldBePlaying && C->GetSound() && !C->IsPlaying())
	{
		C->SetVolumeMultiplier(0.f);
		C->Play();
	}
}

void UDFMusicManagerSubsystem::SetLayerTargetVolumes(
	const float Base,
	const float Combat,
	const float Boss)
{
	TBase = Base;
	TCombat = Combat;
	TBoss = Boss;
}

void UDFMusicManagerSubsystem::SetMusicState(const EMusicState NewState)
{
	if (!ShouldRunMusic())
	{
		return;
	}
	if (NewState == CurrentState)
	{
		return;
	}
	CurrentState = NewState;
	CrossfadeToStateInternal(NewState);
}

void UDFMusicManagerSubsystem::CrossfadeToStateInternal(const EMusicState Target)
{
	ADFMusicLayerHost* const H = Host.Get();
	if (!H || !GetWorld())
	{
		return;
	}
	switch (Target)
	{
	case EMusicState::MainMenu:
		AssignLoopingSoundIfNeeded(H->MusicLayerBase, SoundMainMenu, true);
		AssignLoopingSoundIfNeeded(H->MusicLayerCombat, SoundCombat, false);
		AssignLoopingSoundIfNeeded(H->MusicLayerBoss, SoundBoss, false);
		SetLayerTargetVolumes(1.f, 0.f, 0.f);
		break;
	case EMusicState::Exploration:
		AssignLoopingSoundIfNeeded(H->MusicLayerBase, SoundExplorationBase, true);
		SetLayerTargetVolumes(1.f, 0.f, 0.f);
		break;
	case EMusicState::Combat:
	case EMusicState::Elite:
		AssignLoopingSoundIfNeeded(H->MusicLayerBase, SoundExplorationBase, true);
		AssignLoopingSoundIfNeeded(H->MusicLayerCombat, SoundCombat, true);
		SetLayerTargetVolumes(0.4f, 1.f, 0.f);
		break;
	case EMusicState::Boss:
		AssignLoopingSoundIfNeeded(H->MusicLayerBase, SoundExplorationBase, false);
		AssignLoopingSoundIfNeeded(H->MusicLayerCombat, SoundCombat, false);
		AssignLoopingSoundIfNeeded(H->MusicLayerBoss, SoundBoss, true);
		SetLayerTargetVolumes(0.f, 0.f, 1.f);
		break;
	case EMusicState::Death:
		if (H->MusicLayerBase)
		{
			H->MusicLayerBase->Stop();
		}
		if (H->MusicLayerCombat)
		{
			H->MusicLayerCombat->Stop();
		}
		if (H->MusicLayerBoss)
		{
			H->MusicLayerBoss->Stop();
		}
		CBase = CCombat = CBoss = 0.f;
		TBase = TCombat = TBoss = 0.f;
		PlayDeathSting();
		return;
	case EMusicState::Victory:
		if (H->MusicLayerBase)
		{
			H->MusicLayerBase->Stop();
		}
		if (H->MusicLayerCombat)
		{
			H->MusicLayerCombat->Stop();
		}
		if (H->MusicLayerBoss)
		{
			H->MusicLayerBoss->Stop();
		}
		PlayVictorySting();
		if (UWorld* W = GetWorld())
		{
			W->GetTimerManager().SetTimer(
				VictoryReturnTimer,
				this,
				&UDFMusicManagerSubsystem::OnVictoryReturnToExploration,
				8.f,
				false);
		}
		return;
	}
	StartVolumeLerp();
}

void UDFMusicManagerSubsystem::PlayDeathSting() const
{
	if (!StingDeath || !GetWorld())
	{
		return;
	}
	UGameplayStatics::PlaySound2D(GetWorld(), StingDeath, 1.f, 1.f, 0.f, nullptr, nullptr, true);
}

void UDFMusicManagerSubsystem::PlayVictorySting() const
{
	if (!StingVictory || !GetWorld())
	{
		return;
	}
	UGameplayStatics::PlaySound2D(GetWorld(), StingVictory, 1.f, 1.f, 0.f, nullptr, nullptr, true);
}

void UDFMusicManagerSubsystem::StartVolumeLerp()
{
	if (!GetWorld() || !Host.Get())
	{
		return;
	}
	LerpFrameBudget = 0;
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			CrossfadeLerpHandle,
			this,
			&UDFMusicManagerSubsystem::TickLayerVolumes,
			0.05f,
			true);
	}
}

void UDFMusicManagerSubsystem::TickLayerVolumes()
{
	ADFMusicLayerHost* const H = Host.Get();
	if (!H || !GetWorld())
	{
		return;
	}
	const float Dt = 0.05f;
	const float S = 5.f; // lerp rate
	CBase = FMath::FInterpTo(CBase, TBase, Dt, S);
	CCombat = FMath::FInterpTo(CCombat, TCombat, Dt, S);
	CBoss = FMath::FInterpTo(CBoss, TBoss, Dt, S);
	if (H->MusicLayerBase)
	{
		H->MusicLayerBase->SetVolumeMultiplier(CBase);
	}
	if (H->MusicLayerCombat)
	{
		H->MusicLayerCombat->SetVolumeMultiplier(CCombat);
	}
	if (H->MusicLayerBoss)
	{
		H->MusicLayerBoss->SetVolumeMultiplier(CBoss);
	}
	++LerpFrameBudget;
	if (FMath::IsNearlyEqual(CBase, TBase, 0.02f) && FMath::IsNearlyEqual(CCombat, TCombat, 0.02f) && FMath::IsNearlyEqual(
			CBoss, TBoss, 0.02f))
	{
		if (H->MusicLayerBase)
		{
			H->MusicLayerBase->SetVolumeMultiplier(TBase);
		}
		if (H->MusicLayerCombat)
		{
			H->MusicLayerCombat->SetVolumeMultiplier(TCombat);
		}
		if (H->MusicLayerBoss)
		{
			H->MusicLayerBoss->SetVolumeMultiplier(TBoss);
		}
		if (UWorld* W = GetWorld())
		{
			W->GetTimerManager().ClearTimer(CrossfadeLerpHandle);
		}
	}
	// Failsafe
	if (LerpFrameBudget > 600) // 30s
	{
		if (UWorld* W2 = GetWorld())
		{
			W2->GetTimerManager().ClearTimer(CrossfadeLerpHandle);
		}
	}
}
