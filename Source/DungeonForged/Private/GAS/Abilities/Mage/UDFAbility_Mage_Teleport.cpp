// Source/DungeonForged/Private/GAS/Abilities/Mage/UDFAbility_Mage_Teleport.cpp
#include "GAS/Abilities/Mage/UDFAbility_Mage_Teleport.h"
#include "Camera/UDFLockOnComponent.h"
#include "Characters/ADFPlayerCharacter.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GAS/Effects/UGE_Damage_Magic.h"
#include "GAS/Effects/UGE_Teleport_IFrame.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraFunctionLibrary.h"
#include "NavigationSystem.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"

UDFAbility_Mage_Teleport::UDFAbility_Mage_Teleport()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	AbilityCost_Mana = 40.f;
	BaseCooldown = 10.f;
	TeleportIFrameClass = UGE_Teleport_IFrame::StaticClass();
	SpellstealDamageClass = UGE_Damage_Magic::StaticClass();
}

void UDFAbility_Mage_Teleport::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Mage_Teleport);
	}
}

bool UDFAbility_Mage_Teleport::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) || !ActorInfo)
	{
		return false;
	}
	if (UAbilitySystemComponent* const A = ActorInfo->AbilitySystemComponent.Get())
	{
		if (A->HasMatchingGameplayTag(FDFGameplayTags::State_Dead) || A->HasMatchingGameplayTag(FDFGameplayTags::State_Stunned))
		{
			return false;
		}
	}
	return true;
}

bool UDFAbility_Mage_Teleport::ComputeBlinkDestination(const ACharacter* C, FVector& OutDest) const
{
	OutDest = C->GetActorLocation();
	AActor* Lock = nullptr;
	if (const ADFPlayerCharacter* const P = Cast<ADFPlayerCharacter>(C))
	{
		if (P->LockOnComponent)
		{
			Lock = P->LockOnComponent->GetCurrentTarget();
		}
	}
	if (IsValid(Lock))
	{
		const FVector TLoc = Lock->GetActorLocation();
		const FVector Back = -Lock->GetActorForwardVector() * LockOnOffsetBehind;
		OutDest = TLoc + Back;
		OutDest.Z = C->GetActorLocation().Z;
	}
	else
	{
		FVector Fwd = C->GetActorForwardVector();
		if (APlayerController* const PC = Cast<APlayerController>(C->GetController()))
		{
			FRotator Cam = PC->GetControlRotation();
			Cam.Pitch = 0.f;
			Fwd = Cam.Vector().GetSafeNormal2D();
		}
		OutDest = C->GetActorLocation() + Fwd * BlinkDistance;
	}
	if (UWorld* const W = C->GetWorld())
	{
		if (const UNavigationSystemV1* const Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(W))
		{
			FNavLocation N;
			if (Nav->ProjectPointToNavigation(OutDest, N, FVector(500.f, 500.f, 500.f)))
			{
				OutDest = N.Location;
			}
		}
	}
	return true;
}

void UDFAbility_Mage_Teleport::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo, nullptr))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (UAbilitySystemComponent* const asc = GetAbilitySystemComponentFromActorInfo())
	{
		if (asc->GetOwner() && asc->GetOwner()->HasAuthority())
		{
			ApplyResourceCostsToOwner(asc);
		}
	}
	ACharacter* const C = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo();
	if (!C || !ASC || !C->HasAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	AActor* Lock = nullptr;
	if (const ADFPlayerCharacter* const P = Cast<ADFPlayerCharacter>(C))
	{
		if (P->LockOnComponent)
		{
			Lock = P->LockOnComponent->GetCurrentTarget();
		}
	}
	const bool bTargetCasting = IsValid(Lock) && UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Lock) &&
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Lock)->HasMatchingGameplayTag(FDFGameplayTags::State_Casting);
	FVector Dest = C->GetActorLocation();
	ComputeBlinkDestination(C, Dest);
	if (TeleportIFrameClass)
	{
		const FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
		(void)ASC->ApplyGameplayEffectToSelf(TeleportIFrameClass.GetDefaultObject(), 1.f, Ctx);
	}
	if (DepartureNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, DepartureNiagara, C->GetActorLocation(), FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::None, true);
	}
	C->TeleportTo(Dest, C->GetActorRotation());
	if (ArrivalNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, ArrivalNiagara, C->GetActorLocation(), FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::None, true);
	}
	if (bTargetCasting && IsValid(Lock) && SpellstealDamageClass)
	{
		UAbilitySystemComponent* const Tasc = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Lock);
		if (Tasc)
		{
			const float I = ASC->GetNumericAttribute(UDFAttributeSet::GetIntelligenceAttribute());
			const float Sbc = I * 1.0f; // 0.5+1.0=1.5*I
			FGameplayEffectContextHandle Ctx2 = ASC->MakeEffectContext();
			Ctx2.AddInstigator(C, C);
			Ctx2.AddSourceObject(this);
			const FGameplayEffectSpecHandle Ss = ASC->MakeOutgoingSpec(SpellstealDamageClass, 1.f, Ctx2);
			if (Ss.IsValid() && Ss.Data && FDFGameplayTags::Data_Damage.IsValid())
			{
				Ss.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, Sbc);
				ASC->ApplyGameplayEffectSpecToTarget(*Ss.Data, Tasc);
			}
		}
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
