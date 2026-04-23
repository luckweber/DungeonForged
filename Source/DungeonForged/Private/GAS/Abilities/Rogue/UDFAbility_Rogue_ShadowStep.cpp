// Source/DungeonForged/Private/GAS/Abilities/Rogue/UDFAbility_Rogue_ShadowStep.cpp
#include "GAS/Abilities/Rogue/UDFAbility_Rogue_ShadowStep.h"
#include "Camera/UDFLockOnComponent.h"
#include "Characters/ADFEnemyBase.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Combat/UDFComboPointsComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Rogue_ShadowStep_Stealth.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "GameplayEffect.h"

UDFAbility_Rogue_ShadowStep::UDFAbility_Rogue_ShadowStep()
{
	AbilityCost_Mana = 0.f;
	AbilityCost_Stamina = 30.f;
	BaseCooldown = 12.f;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UDFAbility_Rogue_ShadowStep::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Rogue_ShadowStep);
		AbilityTags.AddTag(FDFGameplayTags::Ability_Rogue);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
	}
}

bool UDFAbility_Rogue_ShadowStep::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) || !ActorInfo)
	{
		return false;
	}
	const ADFPlayerCharacter* const P = Cast<ADFPlayerCharacter>(ActorInfo->AvatarActor.Get());
	if (!P || !P->LockOnComponent || !IsValid(P->LockOnComponent->GetCurrentTarget()))
	{
		return false;
	}
	return true;
}

void UDFAbility_Rogue_ShadowStep::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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
	ACharacter* const C = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	ADFPlayerCharacter* const P = Cast<ADFPlayerCharacter>(C);
	UAbilitySystemComponent* const Src = GetAbilitySystemComponentFromActorInfo();
	if (!C || !P || !Src)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (Src->GetOwner() && Src->GetOwner()->HasAuthority())
	{
		ApplyResourceCostsToOwner(Src);
	}
	AActor* const Target = P->LockOnComponent->GetCurrentTarget();
	if (!IsValid(Target) || !Target->GetRootComponent())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	const FVector Start = C->GetActorLocation();
	if (UWorld* const W = GetWorld())
	{
		if (TrailVFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				this, TrailVFX, Start, C->GetActorRotation(), FVector(1.f), true, true, ENCPoolMethod::AutoRelease, true);
		}
	}
	FGameplayEffectContextHandle Ctx = Src->MakeEffectContext();
	Ctx.AddSourceObject(this);
	const FGameplayEffectSpecHandle S = Src->MakeOutgoingSpec(UGE_Rogue_ShadowStep_Stealth::StaticClass(), 1.f, Ctx);
	if (S.IsValid() && S.Data)
	{
		ShadowStepEffectHandle = Src->ApplyGameplayEffectSpecToSelf(*S.Data);
	}
	const FVector TLoc = Target->GetActorLocation();
	const FVector Back = -Target->GetActorForwardVector() * DistanceBehind;
	FVector Dest = TLoc + Back;
	Dest.Z = C->GetActorLocation().Z + ZOffset;
	if (UWorld* W = C->GetWorld())
	{
		if (const UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(W))
		{
			FNavLocation N;
			if (Nav->ProjectPointToNavigation(Dest, N, FVector(300.f, 300.f, 500.f)))
			{
				Dest = N.Location;
			}
		}
	}
	C->SetActorLocation(Dest, false, nullptr, ETeleportType::TeleportPhysics);
	const FVector ToTarget = (TLoc - C->GetActorLocation()).GetSafeNormal2D();
	if (!ToTarget.IsNearlyZero())
	{
		C->SetActorRotation(ToTarget.Rotation());
	}
	if (UWorld* W = GetWorld())
	{
		if (ArrivalVFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				this, ArrivalVFX, C->GetActorLocation(), C->GetActorRotation(), FVector(1.f), true, true, ENCPoolMethod::AutoRelease, true);
		}
	}
	if (ShadowStepEffectHandle.IsValid())
	{
		Src->RemoveActiveGameplayEffect(ShadowStepEffectHandle, 0);
		ShadowStepEffectHandle = FActiveGameplayEffectHandle();
	}
	if (UDFComboPointsComponent* const Combo = P->ComboPoints)
	{
		Combo->AddComboPoints(2);
	}
	if (Src->GetOwner() && Src->GetOwner()->HasAuthority() && Cast<ADFEnemyBase>(Target) && !Target->IsActorBeingDestroyed())
	{
		FGameplayTagContainer T;
		T.AddTag(FDFGameplayTags::Ability_Rogue_Backstab);
		(void)Src->TryActivateAbilitiesByTag(T, false);
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
