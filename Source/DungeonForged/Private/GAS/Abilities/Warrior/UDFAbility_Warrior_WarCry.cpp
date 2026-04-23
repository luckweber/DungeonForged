// Source/DungeonForged/Private/GAS/Abilities/Warrior/UDFAbility_Warrior_WarCry.cpp
#include "GAS/Abilities/Warrior/UDFAbility_Warrior_WarCry.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Buff_DamageUp.h"
#include "GAS/Effects/UGE_Buff_Speed.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"

FOnDFWarriorWarCry UDFAbility_Warrior_WarCry::OnWarriorWarCry;

static FGenericTeamId DFWarriorGetTeam(AActor* A)
{
	if (!A)
	{
		return FGenericTeamId(0);
	}
	if (const IGenericTeamAgentInterface* T = Cast<IGenericTeamAgentInterface>(A))
	{
		return T->GetGenericTeamId();
	}
	if (const APawn* P = Cast<APawn>(A))
	{
		if (P->GetController() && P->GetController()->IsPlayerController())
		{
			return FGenericTeamId(1u);
		}
	}
	return FGenericTeamId(0);
}

static bool DFWarriorIsAlly(AActor* Caster, AActor* Other)
{
	if (!Caster || !Other)
	{
		return false;
	}
	if (Caster == Other)
	{
		return true;
	}
	const IGenericTeamAgentInterface* T1 = Cast<IGenericTeamAgentInterface>(Caster);
	const IGenericTeamAgentInterface* T2 = Cast<IGenericTeamAgentInterface>(Other);
	if (T1 && T2)
	{
		return T1->GetTeamAttitudeTowards(*Other) == ETeamAttitude::Friendly;
	}
	if (T1)
	{
		return T1->GetTeamAttitudeTowards(*Other) == ETeamAttitude::Friendly;
	}
	return DFWarriorGetTeam(Caster) == DFWarriorGetTeam(Other) && DFWarriorGetTeam(Caster) != 0u;
}

UDFAbility_Warrior_WarCry::UDFAbility_Warrior_WarCry()
{
	AbilityCost_Mana = 30.f;
	AbilityCost_Stamina = 0.f;
	BaseCooldown = 45.f;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UDFAbility_Warrior_WarCry::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Warrior_WarCry);
		ActivationOwnedTags.AddTag(FDFGameplayTags::State_Casting);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Stunned);
	}
}

void UDFAbility_Warrior_WarCry::ActivateAbility(
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
	if (UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo())
	{
		if (asc->GetOwner() && asc->GetOwner()->HasAuthority())
		{
			ApplyResourceCostsToOwner(asc);
		}
	}
	const TObjectPtr<UAnimMontage> M = WarCryMontage ? WarCryMontage : AbilityMontage;
	if (!M)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (UAbilityTask_PlayMontageAndWait* T = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, M, 1.f, NAME_None, true, 1.f, 0.f, true))
	{
		T->OnBlendOut.AddDynamic(this, &UDFAbility_Warrior_WarCry::OnMontageBlended);
		T->OnCompleted.AddDynamic(this, &UDFAbility_Warrior_WarCry::OnMontageFinished);
		T->OnInterrupted.AddDynamic(this, &UDFAbility_Warrior_WarCry::OnMontageFinished);
		T->OnCancelled.AddDynamic(this, &UDFAbility_Warrior_WarCry::OnMontageFinished);
		T->ReadyForActivation();
	}
}

void UDFAbility_Warrior_WarCry::OnMontageBlended()
{
	if (bWarCryApplied)
	{
		return;
	}
	bWarCryApplied = true;
	ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* const Source = GetAbilitySystemComponentFromActorInfo();
	UWorld* W = GetWorld();
	if (!Char || !Source || !W || !Char->HasAuthority())
	{
		return;
	}
	const FVector Loc = Char->GetActorLocation();
	if (OnWarriorWarCry.IsBound())
	{
		OnWarriorWarCry.Broadcast(Char, Loc);
	}
	FCollisionQueryParams Q(SCENE_QUERY_STAT(WarCry), false, Char);
	TArray<FOverlapResult> Hits;
	FCollisionObjectQueryParams Obj(ECC_Pawn);
	W->OverlapMultiByObjectType(
		Hits, Loc, FQuat::Identity, Obj, FCollisionShape::MakeSphere(WarCryRange), Q);
	const TSubclassOf<UGameplayEffect> Dmg = WarCryDamageBuffClass ? WarCryDamageBuffClass : TSubclassOf<UGameplayEffect>(UGE_Buff_DamageUp::StaticClass());
	const TSubclassOf<UGameplayEffect> Spd = WarCrySpeedBuffClass ? WarCrySpeedBuffClass : TSubclassOf<UGameplayEffect>(UGE_Buff_Speed::StaticClass());
	for (FOverlapResult& R : Hits)
	{
		AActor* A = R.GetActor();
		if (!A || !DFWarriorIsAlly(Char, A))
		{
			continue;
		}
		UAbilitySystemComponent* TASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(A);
		if (!TASC)
		{
			continue;
		}
		FGameplayEffectContextHandle Ctx = Source->MakeEffectContext();
		const FGameplayEffectSpecHandle S1 = Source->MakeOutgoingSpec(Dmg, 1.f, Ctx);
		if (S1.IsValid() && S1.Data.IsValid())
		{
			S1.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, 10.f);
			S1.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Magnitude, 25.f);
			Source->ApplyGameplayEffectSpecToTarget(*S1.Data.Get(), TASC);
		}
		const FGameplayEffectSpecHandle S2 = Source->MakeOutgoingSpec(Spd, 1.f, Ctx);
		if (S2.IsValid() && S2.Data.IsValid())
		{
			S2.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, 10.f);
			Source->ApplyGameplayEffectSpecToTarget(*S2.Data.Get(), TASC);
		}
	}
	if (WarCryNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			Char, WarCryNiagara, Loc, FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::None, true);
	}
	UGameplayStatics::PlaySoundAtLocation(this, WarCrySFX, Loc, FRotator::ZeroRotator, 1.f, 1.f, 0.f, nullptr, nullptr, Char);
}

void UDFAbility_Warrior_WarCry::OnMontageFinished()
{
	if (!bWarCryApplied)
	{
		OnMontageBlended();
	}
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
