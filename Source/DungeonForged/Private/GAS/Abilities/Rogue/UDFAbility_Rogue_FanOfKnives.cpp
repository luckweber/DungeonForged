// Source/DungeonForged/Private/GAS/Abilities/Rogue/UDFAbility_Rogue_FanOfKnives.cpp
#include "GAS/Abilities/Rogue/UDFAbility_Rogue_FanOfKnives.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Characters/ADFEnemyBase.h"
#include "Characters/ADFPlayerCharacter.h"
#include "CollisionQueryParams.h"
#include "Combat/ADFKnifeProjectile.h"
#include "Combat/UDFComboPointsComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Engine/OverlapResult.h"
#include "WorldCollision.h"

UDFAbility_Rogue_FanOfKnives::UDFAbility_Rogue_FanOfKnives()
{
	AbilityCost_Mana = 0.f;
	AbilityCost_Stamina = 25.f;
	BaseCooldown = 8.f;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UDFAbility_Rogue_FanOfKnives::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Rogue_FanOfKnives);
		AbilityTags.AddTag(FDFGameplayTags::Ability_Rogue);
		AbilityTags.AddTag(FDFGameplayTags::Ability_Attack_Ranged);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
		ActivationOwnedTags.AddTag(FDFGameplayTags::State_Attacking);
	}
}

UDFComboPointsComponent* UDFAbility_Rogue_FanOfKnives::GetCombo(AActor* const From)
{
	if (const ADFPlayerCharacter* const P = Cast<ADFPlayerCharacter>(From))
	{
		return P->ComboPoints;
	}
	return nullptr;
}

void UDFAbility_Rogue_FanOfKnives::ActivateAbility(
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
	if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (ASC->GetOwner() && ASC->GetOwner()->HasAuthority())
		{
			ApplyResourceCostsToOwner(ASC);
		}
	}
	const TObjectPtr<UAnimMontage> M = FanMontage ? FanMontage : AbilityMontage;
	if (!M)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (UAbilityTask_PlayMontageAndWait* const T = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, M, 1.f, NAME_None, true, 1.f, 0.f, true))
	{
		T->OnCompleted.AddDynamic(this, &UDFAbility_Rogue_FanOfKnives::OnMontageEnd);
		T->OnBlendOut.AddDynamic(this, &UDFAbility_Rogue_FanOfKnives::OnMontageEnd);
		T->OnInterrupted.AddDynamic(this, &UDFAbility_Rogue_FanOfKnives::OnMontageEnd);
		T->OnCancelled.AddDynamic(this, &UDFAbility_Rogue_FanOfKnives::OnMontageEnd);
		T->ReadyForActivation();
	}
	if (UAbilityTask_WaitGameplayEvent* E = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, FDFGameplayTags::Event_Rogue_FanOfKnives_Trace, nullptr, true, true))
	{
		E->EventReceived.AddDynamic(this, &UDFAbility_Rogue_FanOfKnives::OnTraceEvent);
		E->ReadyForActivation();
	}
}

void UDFAbility_Rogue_FanOfKnives::OnTraceEvent(FGameplayEventData /*Payload*/)
{
	SpawnKnives();
}

void UDFAbility_Rogue_FanOfKnives::SpawnKnives()
{
	ACharacter* const C = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!C || !C->HasAuthority() || !GetWorld() || !KnifeClass)
	{
		return;
	}
	UAbilitySystemComponent* const Src = GetAbilitySystemComponentFromActorInfo();
	UDFComboPointsComponent* const Combo = GetCombo(C);
	if (!Src)
	{
		return;
	}
	const float Agi = Src->GetNumericAttribute(UDFAttributeSet::GetAgilityAttribute());
	const float Phys = Agi * 0.6f;
	const float Poi = FMath::Max(2.f, Agi);
	const FVector Center = C->GetActorLocation();
	FCollisionObjectQueryParams Obj(ECC_Pawn);
	Obj.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams Q(SCENE_QUERY_STAT(FanOfKnives), false, C);
	Q.AddIgnoredActor(C);
	TArray<FOverlapResult> O;
	GetWorld()->OverlapMultiByObjectType(O, Center, FQuat::Identity, Obj, FCollisionShape::MakeSphere(FanRadius), Q);
	int32 N = 0;
	for (const FOverlapResult& R : O)
	{
		if (R.GetActor() && R.GetActor()->IsA<ADFEnemyBase>())
		{
			++N;
		}
	}
	N = FMath::Max(1, N);
	for (int32 I = 0; I < N; ++I)
	{
		const float Yaw = (360.f * static_cast<float>(I) / FMath::Max(1, N)) + FMath::FRandRange(-8.f, 8.f);
		const FRotator Dir(0.f, Yaw, 0.f);
		FActorSpawnParameters P;
		P.Instigator = C;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		ADFKnifeProjectile* K = GetWorld()->SpawnActor<ADFKnifeProjectile>(KnifeClass, C->GetActorLocation() + FVector(0, 0, 40.f), Dir, P);
		if (K)
		{
			K->FlightSpeed = 2800.f;
			K->PhysicalHitDamage = Phys;
			K->PoisonMagnitude = Poi;
		}
	}
	if (Combo)
	{
		Combo->AddComboPoints(1);
	}
}

void UDFAbility_Rogue_FanOfKnives::OnMontageEnd()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
