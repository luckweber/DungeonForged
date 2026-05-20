// Source/DungeonForged/Private/FX/UDFCombatFeedbackLibrary.cpp
#include "FX/UDFCombatFeedbackLibrary.h"

#include "GameplayEffect.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Combat/UDFComboComponent.h"
#include "Combat/UDFHitReactionComponent.h"
#include "DFAssetManager.h"
#include "Data/UDFCombatTuningData.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "FX/UDFCameraShakeFunctionLibrary.h"
#include "FX/UDFHitStopSubsystem.h"
#include "FX/UDFScreenEffectsComponent.h"
#include "GameFramework/PlayerController.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "UI/Combat/DFCombatTextTypes.h"
#include "UI/Combat/UDFCombatTextSubsystem.h"
#include "Localization/UDFAccessibilitySubsystem.h"
#include "DungeonForgedModule.h"

namespace
{
FGameplayTag PickImpactTagForSource(const FGameplayTag& SourceSuffix, const FGameplayTag& Light,
	const FGameplayTag& Heavy, const FGameplayTag& Critical, const FGameplayTag& Knockback,
	const EDFHitFeedbackBand Band)
{
	if (!SourceSuffix.IsValid())
	{
		switch (Band)
		{
		case EDFHitFeedbackBand::Knockback: return Knockback;
		case EDFHitFeedbackBand::Critical: return Critical;
		case EDFHitFeedbackBand::Heavy: return Heavy;
		default: return Light;
		}
	}
	if (SourceSuffix.MatchesTag(FDFGameplayTags::Damage_Source_Slash))
	{
		switch (Band)
		{
		case EDFHitFeedbackBand::Knockback: return FDFGameplayTags::Impact_Knockback_Slash;
		case EDFHitFeedbackBand::Critical: return FDFGameplayTags::Impact_Critical_Slash;
		case EDFHitFeedbackBand::Heavy: return FDFGameplayTags::Impact_Heavy_Slash;
		default: return FDFGameplayTags::Impact_Light_Slash;
		}
	}
	if (SourceSuffix.MatchesTag(FDFGameplayTags::Damage_Source_Blunt))
	{
		switch (Band)
		{
		case EDFHitFeedbackBand::Knockback: return FDFGameplayTags::Impact_Knockback_Blunt;
		case EDFHitFeedbackBand::Critical: return FDFGameplayTags::Impact_Critical_Blunt;
		case EDFHitFeedbackBand::Heavy: return FDFGameplayTags::Impact_Heavy_Blunt;
		default: return FDFGameplayTags::Impact_Light_Blunt;
		}
	}
	if (SourceSuffix.MatchesTag(FDFGameplayTags::Damage_Source_Pierce))
	{
		switch (Band)
		{
		case EDFHitFeedbackBand::Knockback: return FDFGameplayTags::Impact_Knockback_Pierce;
		case EDFHitFeedbackBand::Critical: return FDFGameplayTags::Impact_Critical_Pierce;
		case EDFHitFeedbackBand::Heavy: return FDFGameplayTags::Impact_Heavy_Pierce;
		default: return FDFGameplayTags::Impact_Light_Pierce;
		}
	}
	return FGameplayTag();
}

void SpawnImpactFeedbackFromTuning(UObject* const WorldContextObject, const FDFHitConfirmedContext& Context,
	const EDFHitFeedbackBand Band)
{
	if (IsRunningDedicatedServer())
	{
		return;
	}
	const UDFCombatTuningData* const Tuning = UDFAssetManager::Get().GetCombatTuningData();
	if (!Tuning || Tuning->ImpactFeedbackByTag.IsEmpty())
	{
		return;
	}
	FGameplayTag LookupTag = Context.ImpactTag;
	if (!LookupTag.IsValid())
	{
		LookupTag = UDFCombatFeedbackLibrary::ResolveImpactTag(Band, Context.DamageSourceTag);
	}
	const FDFImpactFeedbackAssets* Assets = Tuning->ImpactFeedbackByTag.Find(LookupTag);
	if (!Assets)
	{
		switch (Band)
		{
		case EDFHitFeedbackBand::Knockback: Assets = Tuning->ImpactFeedbackByTag.Find(FDFGameplayTags::Impact_Knockback); break;
		case EDFHitFeedbackBand::Critical: Assets = Tuning->ImpactFeedbackByTag.Find(FDFGameplayTags::Impact_Critical); break;
		case EDFHitFeedbackBand::Heavy: Assets = Tuning->ImpactFeedbackByTag.Find(FDFGameplayTags::Impact_Heavy); break;
		default: Assets = Tuning->ImpactFeedbackByTag.Find(FDFGameplayTags::Impact_Light); break;
		}
	}
	if (!Assets)
	{
		return;
	}
	const FVector SpawnLoc = Context.Location.IsNearlyZero() && Context.Victim
		? Context.Victim->GetActorLocation()
		: Context.Location;
	const FRotator SpawnRot = Context.Normal.IsNearlyZero() ? FRotator::ZeroRotator : Context.Normal.Rotation();
	if (Assets->ImpactVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			WorldContextObject, Assets->ImpactVFX, SpawnLoc, SpawnRot, FVector(1.f), true, true);
	}
	if (Assets->ImpactSFX)
	{
		UGameplayStatics::PlaySoundAtLocation(WorldContextObject, Assets->ImpactSFX, SpawnLoc);
	}
}

ECombatTextType ResolveCombatTextTypeForHit(const FDFHitConfirmedContext& Context, const FGameplayTagContainer& AssetTags)
{
	if (Context.bIsCrit)
	{
		return ECombatTextType::Damage_Critical;
	}
	if (AssetTags.HasTag(FDFGameplayTags::Effect_DoT_Fire) || AssetTags.HasTag(FDFGameplayTags::Effect_DoT_Poison)
		|| AssetTags.HasTag(FDFGameplayTags::Effect_DoT_Bleed) || AssetTags.HasTag(FDFGameplayTags::Effect_DoT_Frost))
	{
		return ECombatTextType::Damage_DoT;
	}
	if (AssetTags.HasTag(FDFGameplayTags::Effect_Damage_True))
	{
		return ECombatTextType::Damage_True;
	}
	if (AssetTags.HasTag(FDFGameplayTags::Effect_Damage_Physical))
	{
		return ECombatTextType::Damage_Physical;
	}
	if (AssetTags.HasTag(FDFGameplayTags::Effect_Damage_Magic))
	{
		return ECombatTextType::Damage_Magic;
	}
	if (Context.DamageSourceTag.MatchesTag(FDFGameplayTags::Damage_Source))
	{
		return ECombatTextType::Damage_Physical;
	}
	return ECombatTextType::Damage_Magic;
}

void SpawnCombatTextFromHitContext(UObject* const WorldContextObject, const FDFHitConfirmedContext& Context)
{
	if (IsRunningDedicatedServer() || !Context.Victim || Context.Magnitude <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	UWorld* const World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	if (!World)
	{
		return;
	}
	if (UGameInstance* const GI = World->GetGameInstance())
	{
		if (const UDFAccessibilitySubsystem* const A11y = GI->GetSubsystem<UDFAccessibilitySubsystem>())
		{
			if (!A11y->GetSettings().bShowDamageNumbers)
			{
				return;
			}
		}
	}
	UDFCombatTextSubsystem* const Ctx = World->GetSubsystem<UDFCombatTextSubsystem>();
	if (!Ctx)
	{
		return;
	}
	const FVector Loc = Context.Location.IsNearlyZero()
		? Context.Victim->GetActorLocation() + FVector(0.f, 0.f, 70.f)
		: Context.Location + FVector(0.f, 0.f, 40.f);
	const ECombatTextType TextType = ResolveCombatTextTypeForHit(Context, Context.Tags);
	Ctx->SpawnText(Loc, Context.Magnitude, TextType);
}
} // namespace

EDFHitFeedbackBand UDFCombatFeedbackLibrary::ResolveFeedbackBand(
	const float DamageMagnitude,
	const float MaxHealth,
	const bool bIsCrit,
	const bool bIsKnockback,
	const float HeavyDamageThreshold)
{
	if (bIsKnockback)
	{
		return EDFHitFeedbackBand::Knockback;
	}
	if (bIsCrit)
	{
		return EDFHitFeedbackBand::Critical;
	}
	const float Pct = MaxHealth > KINDA_SMALL_NUMBER ? (DamageMagnitude / MaxHealth) : 0.f;
	if (Pct > 0.3f)
	{
		return EDFHitFeedbackBand::Critical;
	}
	if (DamageMagnitude >= HeavyDamageThreshold)
	{
		return EDFHitFeedbackBand::Heavy;
	}
	return EDFHitFeedbackBand::Light;
}

FGameplayTag UDFCombatFeedbackLibrary::ResolveImpactTag(
	const EDFHitFeedbackBand Band,
	const FGameplayTag DamageSourceTag)
{
	return PickImpactTagForSource(
		DamageSourceTag,
		FDFGameplayTags::Impact_Light,
		FDFGameplayTags::Impact_Heavy,
		FDFGameplayTags::Impact_Critical,
		FDFGameplayTags::Impact_Knockback,
		Band);
}

void UDFCombatFeedbackLibrary::PlayHitFeedbackForBand(
	UObject* const WorldContextObject,
	const EDFHitFeedbackBand Band,
	AActor* const InstigatorActor)
{
	UWorld* const World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (UDFHitStopSubsystem* const HS = World->GetSubsystem<UDFHitStopSubsystem>())
	{
		HS->PlayBand(Band, InstigatorActor, 1.f);
	}
	if (APlayerController* const PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		switch (Band)
		{
		case EDFHitFeedbackBand::Light:
			UDFCameraShakeFunctionLibrary::PlayLightHitOnOwner(WorldContextObject, PC);
			break;
		case EDFHitFeedbackBand::Heavy:
		case EDFHitFeedbackBand::Critical:
			UDFCameraShakeFunctionLibrary::PlayHeavyHitOnOwner(WorldContextObject, PC);
			break;
		case EDFHitFeedbackBand::Knockback:
			UDFCameraShakeFunctionLibrary::PlayBossSlamOnOwner(WorldContextObject, PC);
			break;
		default:
			break;
		}
	}
}

void UDFCombatFeedbackLibrary::PlayHitFeedbackFromDamage(
	UObject* const WorldContextObject,
	const float DamageMagnitude,
	const float MaxHealth,
	const bool bIsKnockback,
	AActor* const InstigatorActor)
{
	const EDFHitFeedbackBand Band = ResolveFeedbackBand(DamageMagnitude, MaxHealth, false, bIsKnockback);
	PlayHitFeedbackForBand(WorldContextObject, Band, InstigatorActor);
}

void UDFCombatFeedbackLibrary::DispatchAttackerHitFeel(
	UObject* const WorldContextObject,
	const FDFHitConfirmedContext& Context)
{
	if (IsRunningDedicatedServer())
	{
		return;
	}
	UWorld* const World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	if (!World || !Context.Instigator)
	{
		return;
	}
	float MaxH = Context.MaxHealth;
	if (MaxH <= KINDA_SMALL_NUMBER && Context.Victim)
	{
		if (UAbilitySystemComponent* const VictimASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Context.Victim))
		{
			MaxH = FMath::Max(1.f, VictimASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute()));
		}
	}
	const float DamagePct = MaxH > KINDA_SMALL_NUMBER ? (Context.Magnitude / MaxH) : Context.DamagePercent;
	const bool bKnockback = Context.KnockbackMagnitude >= 60.f
		|| Context.Band == EDFHitFeedbackBand::Knockback;
	EDFHitFeedbackBand Band = Context.Band;
	if (Context.bWasLethal)
	{
		Band = EDFHitFeedbackBand::Critical;
	}
	else if (Band == EDFHitFeedbackBand::Light)
	{
		Band = ResolveFeedbackBand(Context.Magnitude, MaxH, Context.bIsCrit, bKnockback);
	}
	const float MagFactor = FMath::Clamp(DamagePct / 0.15f, 0.5f, 1.5f);

	if (UDFHitStopSubsystem* const HS = World->GetSubsystem<UDFHitStopSubsystem>())
	{
		HS->PlayBand(Band, Context.Instigator, MagFactor);
	}

	if (const APawn* const InstPawn = Cast<APawn>(Context.Instigator))
	{
		if (APlayerController* const InstPC = Cast<APlayerController>(InstPawn->GetController()))
		{
			if (InstPC->IsLocalController())
			{
				switch (Band)
				{
				case EDFHitFeedbackBand::Light:
					UDFCameraShakeFunctionLibrary::PlayLightHitOnOwner(WorldContextObject, InstPC);
					break;
				case EDFHitFeedbackBand::Heavy:
				case EDFHitFeedbackBand::Critical:
					UDFCameraShakeFunctionLibrary::PlayHeavyHitOnOwner(WorldContextObject, InstPC);
					break;
				case EDFHitFeedbackBand::Knockback:
					UDFCameraShakeFunctionLibrary::PlayBossSlamOnOwner(WorldContextObject, InstPC);
					break;
				default:
					break;
				}
			}
		}
	}

	FDFHitConfirmedContext ImpactCtx = Context;
	if (!ImpactCtx.ImpactTag.IsValid())
	{
		ImpactCtx.ImpactTag = ResolveImpactTag(Band, Context.DamageSourceTag);
	}
	SpawnImpactFeedbackFromTuning(WorldContextObject, ImpactCtx, Band);
}

void UDFCombatFeedbackLibrary::DispatchVictimHitFeel(
	UObject* const WorldContextObject,
	const FDFHitConfirmedContext& Context)
{
	if (IsRunningDedicatedServer())
	{
		return;
	}
	float MaxH = Context.MaxHealth;
	if (MaxH <= KINDA_SMALL_NUMBER && Context.Victim)
	{
		if (UAbilitySystemComponent* const VictimASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Context.Victim))
		{
			MaxH = FMath::Max(1.f, VictimASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute()));
		}
	}
	const float DamagePct = MaxH > KINDA_SMALL_NUMBER ? (Context.Magnitude / MaxH) : Context.DamagePercent;
	const bool bKnockback = Context.KnockbackMagnitude >= 60.f
		|| Context.Band == EDFHitFeedbackBand::Knockback;
	EDFHitFeedbackBand Band = Context.Band;
	if (Band == EDFHitFeedbackBand::Light)
	{
		Band = ResolveFeedbackBand(Context.Magnitude, MaxH, Context.bIsCrit, bKnockback);
	}

	if (ADFPlayerCharacter* const VictimPlayer = Cast<ADFPlayerCharacter>(Context.Victim))
	{
		if (VictimPlayer->IsLocallyControlled() && VictimPlayer->ScreenEffects)
		{
			VictimPlayer->ScreenEffects->ApplyHitFromCombat(
				Band,
				DamagePct,
				Context.Instigator,
				VictimPlayer->GetController<APlayerController>());
		}
	}
}

void UDFCombatFeedbackLibrary::DispatchOnHitConfirmed(
	UObject* const WorldContextObject,
	const FDFHitConfirmedContext& Context)
{
	UWorld* const World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	if (!World || !Context.Victim || Context.Magnitude <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const bool bRunGameplay = World->GetNetMode() != NM_Client;
	if (bRunGameplay)
	{
		FVector HitDir = Context.HitDirection2D;
		if (HitDir.IsNearlyZero())
		{
			if (Context.Instigator)
			{
				HitDir = Context.Victim->GetActorLocation() - Context.Instigator->GetActorLocation();
				HitDir.Z = 0.f;
				HitDir.Normalize();
			}
		}
		if (UDFHitReactionComponent* const HitReact = Context.Victim->FindComponentByClass<UDFHitReactionComponent>())
		{
			HitReact->OnHitReceived(
				Context.Magnitude,
				Context.KnockbackMagnitude,
				HitDir,
				Context.Instigator,
				Context.Location,
				Context.Normal,
				Context.DamageSourceTag,
				Context.HitBoneName);
		}
		if (Context.Instigator)
		{
			if (UDFComboComponent* const Combo = Context.Instigator->FindComponentByClass<UDFComboComponent>())
			{
				Combo->NotifyOwnerHitConfirmed();
			}
		}
		if (ADFPlayerCharacter* const Attacker = Cast<ADFPlayerCharacter>(Context.Instigator))
		{
			if (Attacker->HasAuthority() && Attacker->GetController() && !Attacker->IsLocallyControlled())
			{
				FDFHitConfirmedContext FeelCtx = Context;
				if (!FeelCtx.ImpactTag.IsValid())
				{
					EDFHitFeedbackBand Band = Context.Band;
					if (Band == EDFHitFeedbackBand::Light)
					{
						const float MaxH = Context.MaxHealth > KINDA_SMALL_NUMBER ? Context.MaxHealth : 1.f;
						const bool bKnockback = Context.KnockbackMagnitude >= 60.f;
						Band = ResolveFeedbackBand(Context.Magnitude, MaxH, Context.bIsCrit, bKnockback);
					}
					FeelCtx.ImpactTag = ResolveImpactTag(Band, Context.DamageSourceTag);
				}
				Attacker->Client_OnAttackHitConfirmed(FeelCtx);
			}
		}
	}

	if (World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	FDFHitConfirmedContext LocalCtx = Context;
	if (!LocalCtx.bWasLethal && Context.Victim)
	{
		if (UAbilitySystemComponent* const VictimASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Context.Victim))
		{
			if (VictimASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute()) <= KINDA_SMALL_NUMBER)
			{
				LocalCtx.bWasLethal = true;
			}
		}
	}
	if (!LocalCtx.ImpactTag.IsValid())
	{
		float MaxH = Context.MaxHealth;
		if (MaxH <= KINDA_SMALL_NUMBER)
		{
			if (UAbilitySystemComponent* const VictimASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Context.Victim))
			{
				MaxH = FMath::Max(1.f, VictimASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute()));
			}
		}
		const bool bKnockback = Context.KnockbackMagnitude >= 60.f || Context.Band == EDFHitFeedbackBand::Knockback;
		EDFHitFeedbackBand Band = Context.Band;
		if (Band == EDFHitFeedbackBand::Light)
		{
			Band = ResolveFeedbackBand(Context.Magnitude, MaxH, Context.bIsCrit, bKnockback);
		}
		LocalCtx.ImpactTag = ResolveImpactTag(Band, Context.DamageSourceTag);
	}

	DispatchAttackerHitFeel(WorldContextObject, LocalCtx);
	DispatchVictimHitFeel(WorldContextObject, LocalCtx);
	SpawnCombatTextFromHitContext(WorldContextObject, LocalCtx);

	UE_LOG(LogDFFeel, Verbose,
		TEXT("[OnHit] Band=%s Mag=%.1f Crit=%d Inst=%s Vic=%s Impact=%s Tags=[%s]"),
		*UEnum::GetValueAsString(LocalCtx.Band),
		Context.Magnitude,
		Context.bIsCrit ? 1 : 0,
		*GetNameSafe(Context.Instigator),
		*GetNameSafe(Context.Victim),
		*LocalCtx.ImpactTag.ToString(),
		*Context.Tags.ToStringSimple());
}

void UDFCombatFeedbackLibrary::DispatchProjectileHitConfirmed(
	UObject* const WorldContextObject,
	AActor* const Instigator,
	AActor* const Victim,
	const FHitResult& Hit,
	const float DamageMagnitude,
	const float KnockbackMagnitude,
	const FGameplayTag DamageSourceTag,
	const bool bIsCrit)
{
	if (!Victim || DamageMagnitude <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	float MaxH = 1.f;
	if (UAbilitySystemComponent* const VictimASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Victim))
	{
		MaxH = FMath::Max(1.f, VictimASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute()));
	}
	FVector HitDir = Victim->GetActorLocation();
	if (Instigator)
	{
		HitDir = Victim->GetActorLocation() - Instigator->GetActorLocation();
	}
	HitDir.Z = 0.f;
	HitDir.Normalize();

	const EDFHitFeedbackBand Band = ResolveFeedbackBand(DamageMagnitude, MaxH, bIsCrit, KnockbackMagnitude >= 60.f);

	FDFHitConfirmedContext Ctx;
	Ctx.Instigator = Instigator;
	Ctx.Victim = Victim;
	Ctx.Location = Hit.bBlockingHit ? FVector(Hit.ImpactPoint) : Victim->GetActorLocation();
	Ctx.Normal = Hit.bBlockingHit ? FVector(Hit.ImpactNormal) : FVector::UpVector;
	Ctx.HitDirection2D = HitDir;
	Ctx.Magnitude = DamageMagnitude;
	Ctx.KnockbackMagnitude = KnockbackMagnitude;
	Ctx.MaxHealth = MaxH;
	Ctx.DamagePercent = MaxH > KINDA_SMALL_NUMBER ? (DamageMagnitude / MaxH) : 0.f;
	Ctx.bIsCrit = bIsCrit;
	Ctx.DamageSourceTag = DamageSourceTag;
	Ctx.ImpactTag = ResolveImpactTag(Band, DamageSourceTag);
	if (DamageSourceTag.IsValid())
	{
		Ctx.Tags.AddTag(DamageSourceTag);
	}
	if (Ctx.ImpactTag.IsValid())
	{
		Ctx.Tags.AddTag(Ctx.ImpactTag);
	}
	Ctx.Band = Band;
	DispatchOnHitConfirmed(WorldContextObject, Ctx);
}

void UDFCombatFeedbackLibrary::MarkSpecCombatFeedbackCentralized(FGameplayEffectSpec& Spec)
{
	if (FDFGameplayTags::Effect_CombatFeedbackCentralized.IsValid())
	{
		Spec.AddDynamicAssetTag(FDFGameplayTags::Effect_CombatFeedbackCentralized);
	}
}
