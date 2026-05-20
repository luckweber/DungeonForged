// Source/DungeonForged/Private/Combat/UDFDeathCinematicSubsystem.cpp
#include "Combat/UDFDeathCinematicSubsystem.h"

#include "Combat/UDFCombatSpectacleSubsystem.h"

#include "Audio/UDFMusicManagerSubsystem.h"
#include "Characters/ADFEnemyBase.h"
#include "Characters/ADFPlayerCharacter.h"
#include "DungeonForgedModule.h"
#include "FX/UDFCameraShakeFunctionLibrary.h"
#include "FX/UDFHitStopSubsystem.h"
#include "FX/UDFScreenEffectsComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Combat/DFCombatTextTypes.h"
#include "UI/Combat/UDFCombatTextSubsystem.h"

void UDFDeathCinematicSubsystem::PlayPlayerDeathCinematic(ADFPlayerCharacter* const Player)
{
	if (!Player || IsRunningDedicatedServer() || !Player->IsLocallyControlled())
	{
		return;
	}
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	UE_LOG(LogDFDeath, Verbose, TEXT("[Death] PlayerDeathCinematic %s"), *GetNameSafe(Player));

	if (Player->ScreenEffects)
	{
		Player->ScreenEffects->OnDeath();
	}
	if (UDFHitStopSubsystem* const HS = World->GetSubsystem<UDFHitStopSubsystem>())
	{
		HS->BossSlam(nullptr);
	}
	if (UDFMusicManagerSubsystem* const Music = World->GetSubsystem<UDFMusicManagerSubsystem>())
	{
		Music->PlayDeathSting();
	}
	if (APlayerController* const PC = Player->GetController<APlayerController>())
	{
		UDFCameraShakeFunctionLibrary::PlayBossSlamOnOwner(Player, PC);
	}
}

void UDFDeathCinematicSubsystem::PlayEnemyKillCinematic(const FDFDeathCinematicContext& Context)
{
	if (IsRunningDedicatedServer() || !Context.Victim)
	{
		return;
	}
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	UE_LOG(LogDFDeath, Verbose,
		TEXT("[Death] EnemyKillCinematic Victim=%s Killer=%s XP=%.0f LastInRoom=%d"),
		*GetNameSafe(Context.Victim),
		*GetNameSafe(Context.Killer),
		Context.ExperienceReward,
		Context.bIsLastEnemyOfRoom ? 1 : 0);

	if (Context.Killer)
	{
		if (UDFHitStopSubsystem* const HS = World->GetSubsystem<UDFHitStopSubsystem>())
		{
			HS->CriticalHit(Context.Killer);
		}
		if (const APawn* const KillerPawn = Cast<APawn>(Context.Killer))
		{
			if (APlayerController* const PC = Cast<APlayerController>(KillerPawn->GetController()))
			{
				if (PC->IsLocalController())
				{
					UDFCameraShakeFunctionLibrary::PlayHeavyHitOnOwner(Context.Victim, PC);
				}
			}
		}
	}

	if (Context.ExperienceReward > KINDA_SMALL_NUMBER && Context.Killer)
	{
		if (UDFCombatTextSubsystem* const CT = World->GetSubsystem<UDFCombatTextSubsystem>())
		{
			const FVector Loc = Context.LethalImpactLocation.IsNearlyZero()
				? Context.Victim->GetActorLocation() + FVector(0.f, 0.f, 120.f)
				: Context.LethalImpactLocation + FVector(0.f, 0.f, 80.f);
			CT->SpawnText(Loc, Context.ExperienceReward, ECombatTextType::XPGain);
		}
	}
}

void UDFDeathCinematicSubsystem::PlayLastEnemyKillCinematic(AActor* const Killer, AActor* const LastVictim)
{
	if (IsRunningDedicatedServer())
	{
		return;
	}
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	UE_LOG(LogDFDeath, Verbose,
		TEXT("[Death] LastEnemyKillCinematic Victim=%s Killer=%s"),
		*GetNameSafe(LastVictim),
		*GetNameSafe(Killer));

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* const PC = It->Get())
		{
			if (ADFPlayerCharacter* const P = Cast<ADFPlayerCharacter>(PC->GetPawn()))
			{
				if (P->ScreenEffects)
				{
					P->ScreenEffects->FlashScreen(FLinearColor(1.f, 0.95f, 0.8f, 0.3f), 0.4f, 0.35f);
				}
			}
		}
	}
	if (UDFCombatSpectacleSubsystem* const Spec = World->GetSubsystem<UDFCombatSpectacleSubsystem>())
	{
		Spec->PlayLastKillSpectacle(LastVictim, Killer);
	}
}

void UDFDeathCinematicSubsystem::ClearDeathTimeEffects()
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}
	UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
	if (UDFHitStopSubsystem* const HS = World->GetSubsystem<UDFHitStopSubsystem>())
	{
		HS->ForceEndHitStop();
	}
}

FString UDFDeathCinematicSubsystem::ResolveLethalCauseString(
	AActor* const Victim,
	AActor* const Instigator,
	AActor* const Causer,
	const FGameplayTagContainer& Tags)
{
	if (!Victim)
	{
		return FString();
	}
	if (Instigator == Victim || Causer == Victim)
	{
		return FString();
	}
	if (const ADFEnemyBase* const Enemy = Cast<ADFEnemyBase>(Instigator ? Instigator : Causer))
	{
		const FText Display = Enemy->GetEnemyDisplayName();
		if (!Display.IsEmpty())
		{
			return FString::Printf(TEXT("Killed by %s"), *Display.ToString());
		}
	}
	if (FDFGameplayTags::Effect_DoT_Bleed.IsValid() && Tags.HasTag(FDFGameplayTags::Effect_DoT_Bleed))
	{
		return TEXT("Killed by Bleed");
	}
	if (FDFGameplayTags::Effect_DoT_Frost.IsValid() && Tags.HasTag(FDFGameplayTags::Effect_DoT_Frost))
	{
		return TEXT("Killed by Frost");
	}
	if (Instigator)
	{
		return FString::Printf(TEXT("Killed by %s"), *Instigator->GetName());
	}
	if (Causer)
	{
		return FString::Printf(TEXT("Killed by %s"), *Causer->GetName());
	}
	return FString();
}
