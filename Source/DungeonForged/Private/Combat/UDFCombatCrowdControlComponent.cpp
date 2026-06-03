// Source/DungeonForged/Private/Combat/UDFCombatCrowdControlComponent.cpp
#include "Combat/UDFCombatCrowdControlComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Combat/UDFHitReactionComponent.h"
#include "Combat/UDFStaggerComponent.h"
#include "Data/UDFCombatTuningData.h"
#include "DFAssetManager.h"
#include "FX/UDFCombatFeedbackTypes.h"
#include "GAS/DFGameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

UDFCombatCrowdControlComponent::UDFCombatCrowdControlComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDFCombatCrowdControlComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyTuningFromDataAsset();
	CacheSiblingComponents();
}

void UDFCombatCrowdControlComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(JuggleResetTimer);
		W->GetTimerManager().ClearTimer(GravityRestoreTimer);
	}
	EndJuggle();
	Super::EndPlay(EndPlayReason);
}

void UDFCombatCrowdControlComponent::ApplyTuningFromDataAsset()
{
	const UDFCombatTuningData* const Tuning = UDFAssetManager::Get().GetCombatTuningData();
	if (!Tuning)
	{
		return;
	}
	MaxJuggleHits = Tuning->MaxJuggleHitsPerTarget;
	JuggleCountResetSeconds = Tuning->JuggleCountResetSeconds;
	KnockbackMagnitudeThreshold = Tuning->KnockbackMagnitudeThreshold;
	HeavyFlinchDamageThreshold = Tuning->HeavyFlinchDamageThreshold;
}

void UDFCombatCrowdControlComponent::CacheSiblingComponents()
{
	AActor* const Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	HitReaction = Owner->FindComponentByClass<UDFHitReactionComponent>();
	Stagger = Owner->FindComponentByClass<UDFStaggerComponent>();
	if (HitReaction)
	{
		HeavyFlinchDamageThreshold = HitReaction->StaggerThreshold;
		KnockbackDamageFallbackThreshold = HitReaction->KnockbackThreshold;
	}
}

bool UDFCombatCrowdControlComponent::CanReceiveLaunch() const
{
	if (MaxJuggleHits <= 0)
	{
		return true;
	}
	return JuggleHitCount < MaxJuggleHits;
}

EDFCrowdControlTier UDFCombatCrowdControlComponent::ResolveTier(const FDFHitConfirmedContext& Context) const
{
	if (UAbilitySystemComponent* const ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Context.Victim))
	{
		if (FDFGameplayTags::State_CCIgnore.IsValid() && ASC->HasMatchingGameplayTag(FDFGameplayTags::State_CCIgnore))
		{
			return EDFCrowdControlTier::Suppressed;
		}
		if (FDFGameplayTags::State_Invulnerable.IsValid() && ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Invulnerable))
		{
			return EDFCrowdControlTier::Suppressed;
		}
		if (FDFGameplayTags::State_Stunned.IsValid() && ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Stunned))
		{
			if (Stagger && Stagger->IsOnStaggerCooldown())
			{
				return EDFCrowdControlTier::FlinchLight;
			}
		}
	}

	if (bIsJuggled)
	{
		return EDFCrowdControlTier::Juggle;
	}

	const bool bHasKnockbackMag = Context.KnockbackMagnitude > KINDA_SMALL_NUMBER;
	const bool bKnockback = bHasKnockbackMag
		? Context.KnockbackMagnitude >= KnockbackMagnitudeThreshold
		: (Context.Magnitude >= KnockbackDamageFallbackThreshold
			|| Context.Band == EDFHitFeedbackBand::Knockback);
	if (bKnockback && !(bIsJuggled && bSuppressKnockbackWhileJuggled))
	{
		return EDFCrowdControlTier::Knockback;
	}

	if (Context.Magnitude >= HeavyFlinchDamageThreshold)
	{
		return EDFCrowdControlTier::FlinchHeavy;
	}
	return EDFCrowdControlTier::FlinchLight;
}

EDFCrowdControlTier UDFCombatCrowdControlComponent::ProcessCombatHit(const FDFHitConfirmedContext& Context)
{
	AActor* const Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !Context.Victim || Context.Magnitude <= KINDA_SMALL_NUMBER)
	{
		return EDFCrowdControlTier::Suppressed;
	}

	CacheSiblingComponents();
	const EDFCrowdControlTier Tier = ResolveTier(Context);
	if (Tier == EDFCrowdControlTier::Suppressed)
	{
		return Tier;
	}

	if (Tier == EDFCrowdControlTier::Juggle)
	{
		++JuggleHitCount;
		if (MaxJuggleHits > 0 && JuggleHitCount >= MaxJuggleHits)
		{
			ScheduleJuggleCountReset();
		}
	}

	if (HitReaction)
	{
		float KnockbackForReact = Context.KnockbackMagnitude;
		if (bIsJuggled && bSuppressKnockbackWhileJuggled)
		{
			KnockbackForReact = 0.f;
		}
		HitReaction->OnHitReceived(
			Context.Magnitude,
			KnockbackForReact,
			Context.HitDirection2D,
			Context.Instigator,
			Context.Location,
			Context.Normal,
			Context.DamageSourceTag,
			Context.HitBoneName);
	}

	return Tier;
}

bool UDFCombatCrowdControlComponent::TryReceiveLaunch(
	const FVector LaunchVelocity,
	const float TargetGravityScale,
	const float HangtimeSeconds,
	AActor* const Instigator)
{
	AActor* const Owner = GetOwner();
	ACharacter* const VictimChar = Cast<ACharacter>(Owner);
	if (!Owner || !Owner->HasAuthority() || !VictimChar)
	{
		return false;
	}
	if (!CanReceiveLaunch())
	{
		return false;
	}

	bIsJuggled = true;
	SyncJuggleTags(true);
	ScheduleJuggleCountReset();

	const FVector WorldVel = Instigator
		? Instigator->GetActorTransform().TransformVectorNoScale(LaunchVelocity)
		: LaunchVelocity;
	VictimChar->LaunchCharacter(WorldVel, true, true);

	if (UCharacterMovementComponent* const CMC = VictimChar->GetCharacterMovement())
	{
		if (TargetGravityScale > 0.f && TargetGravityScale < 1.f && HangtimeSeconds > 0.f)
		{
			SavedGravityScale = CMC->GravityScale;
			CMC->GravityScale = TargetGravityScale;
			if (UWorld* const W = GetWorld())
			{
				W->GetTimerManager().ClearTimer(GravityRestoreTimer);
				W->GetTimerManager().SetTimer(
					GravityRestoreTimer,
					this,
					&UDFCombatCrowdControlComponent::RestoreSavedGravity,
					HangtimeSeconds,
					false);
			}
		}
	}

	return true;
}

void UDFCombatCrowdControlComponent::RestoreSavedGravity()
{
	if (ACharacter* const Char = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* const CMC = Char->GetCharacterMovement())
		{
			if (SavedGravityScale >= 0.f)
			{
				CMC->GravityScale = SavedGravityScale;
				SavedGravityScale = -1.f;
			}
		}
	}
}

void UDFCombatCrowdControlComponent::ScheduleJuggleCountReset()
{
	if (JuggleCountResetSeconds <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	W->GetTimerManager().ClearTimer(JuggleResetTimer);
	W->GetTimerManager().SetTimer(
		JuggleResetTimer,
		this,
		&UDFCombatCrowdControlComponent::EndJuggle,
		JuggleCountResetSeconds,
		false);
}

void UDFCombatCrowdControlComponent::SyncJuggleTags(const bool bActive)
{
	UAbilitySystemComponent* const ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!ASC || !FDFGameplayTags::State_Juggled.IsValid())
	{
		return;
	}
	if (bActive)
	{
		ASC->AddLooseGameplayTag(FDFGameplayTags::State_Juggled);
	}
	else
	{
		ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Juggled);
	}
}

void UDFCombatCrowdControlComponent::EndJuggle()
{
	bIsJuggled = false;
	JuggleHitCount = 0;
	SyncJuggleTags(false);
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(JuggleResetTimer);
	}
}

void UDFCombatCrowdControlComponent::OnOwnerLanded()
{
	RestoreSavedGravity();
	if (bIsJuggled)
	{
		EndJuggle();
	}
}
