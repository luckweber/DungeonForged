// Source/DungeonForged/Private/GAS/Abilities/Rogue/UDFAbility_Rogue_SmokeScreen.cpp
#include "GAS/Abilities/Rogue/UDFAbility_Rogue_SmokeScreen.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Combat/ADFSmokeBombActor.h"
#include "GAS/DFGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

UDFAbility_Rogue_SmokeScreen::UDFAbility_Rogue_SmokeScreen()
{
	AbilityCost_Mana = 0.f;
	AbilityCost_Stamina = 20.f;
	BaseCooldown = 20.f;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UDFAbility_Rogue_SmokeScreen::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Rogue_SmokeScreen);
		AbilityTags.AddTag(FDFGameplayTags::Ability_Rogue);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
		ActivationOwnedTags.AddTag(FDFGameplayTags::State_Attacking);
	}
}

void UDFAbility_Rogue_SmokeScreen::ActivateAbility(
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
	const TObjectPtr<UAnimMontage> M = ThrowMontage ? ThrowMontage : AbilityMontage;
	if (M)
	{
		if (UAbilityTask_PlayMontageAndWait* const T = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, NAME_None, M, 1.f, NAME_None, true, 0.2f, 0.f, true))
		{
			T->OnCompleted.AddDynamic(this, &UDFAbility_Rogue_SmokeScreen::OnMontageEnd);
			T->OnBlendOut.AddDynamic(this, &UDFAbility_Rogue_SmokeScreen::OnMontageEnd);
			T->OnInterrupted.AddDynamic(this, &UDFAbility_Rogue_SmokeScreen::OnMontageEnd);
			T->OnCancelled.AddDynamic(this, &UDFAbility_Rogue_SmokeScreen::OnMontageEnd);
			T->ReadyForActivation();
		}
	}
	if (ACharacter* const C = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (C->HasAuthority() && GetWorld() && SmokeBombClass)
		{
			FVector Fwd = C->GetActorForwardVector();
			if (APlayerController* const PC = Cast<APlayerController>(C->GetController()))
			{
				FRotator R = PC->GetControlRotation();
				R.Pitch = 0.f;
				Fwd = R.Vector().GetSafeNormal2D();
			}
			const FVector Dest = C->GetActorLocation() + Fwd * ThrowDistance;
			FActorSpawnParameters P;
			P.Instigator = C;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			(void)GetWorld()->SpawnActor<ADFSmokeBombActor>(SmokeBombClass, Dest, Fwd.Rotation(), P);
		}
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UDFAbility_Rogue_SmokeScreen::OnMontageEnd()
{
	// Fire-and-forget: no-op (smoke spawns in Activate). Montage is cosmetic.
}
