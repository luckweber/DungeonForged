// Source/DungeonForged/Private/Combat/UDFCombatInterruptLibrary.cpp
#include "Combat/UDFCombatInterruptLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Characters/ADFEnemyBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "DungeonForgedModule.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffect.h"

bool UDFCombatInterruptLibrary::TryInterruptBossCast(
	UObject* const WorldContextObject,
	AActor* const Target,
	AActor* const Instigator,
	const TSubclassOf<UGameplayEffect> BonusStunEffect,
	const float BonusStunDuration)
{
	(void)WorldContextObject;
	if (!IsValid(Target) || !FDFGameplayTags::State_Combat_Casting_Interruptible.IsValid())
	{
		return false;
	}
	UAbilitySystemComponent* const TASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TASC || !TASC->HasMatchingGameplayTag(FDFGameplayTags::State_Combat_Casting_Interruptible))
	{
		return false;
	}

	FGameplayTagContainer WithoutDeath;
	if (FDFGameplayTags::Ability_Death.IsValid())
	{
		WithoutDeath.AddTag(FDFGameplayTags::Ability_Death);
	}
	if (FDFGameplayTags::Ability_Death_Enemy.IsValid())
	{
		WithoutDeath.AddTag(FDFGameplayTags::Ability_Death_Enemy);
	}
	TASC->CancelAbilities(nullptr, WithoutDeath.Num() > 0 ? &WithoutDeath : nullptr);

	if (ACharacter* const Char = Cast<ACharacter>(Target))
	{
		if (USkeletalMeshComponent* const Mesh = Char->GetMesh())
		{
			if (UAnimInstance* const Anim = Mesh->GetAnimInstance())
			{
				Anim->Montage_Stop(0.15f);
			}
		}
	}

	TASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Combat_Casting_Interruptible);

	if (BonusStunEffect && BonusStunDuration > KINDA_SMALL_NUMBER)
	{
		FGameplayEffectContextHandle Ctx = TASC->MakeEffectContext();
		if (Instigator)
		{
			Ctx.AddInstigator(Instigator, Instigator);
		}
		const FGameplayEffectSpecHandle Spec = TASC->MakeOutgoingSpec(BonusStunEffect, 1.f, Ctx);
		if (Spec.IsValid() && Spec.Data)
		{
			if (FDFGameplayTags::Data_Duration.IsValid())
			{
				Spec.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, BonusStunDuration);
			}
			TASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	if (FDFGameplayTags::State_BossVulnerable.IsValid())
	{
		TASC->AddLooseGameplayTag(FDFGameplayTags::State_BossVulnerable, 1);
	}

	if (FDFGameplayTags::Event_Combat_Boss_Interrupted.IsValid())
	{
		FGameplayEventData Payload;
		Payload.EventTag = FDFGameplayTags::Event_Combat_Boss_Interrupted;
		Payload.Instigator = Instigator;
		Payload.Target = Target;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Target, FDFGameplayTags::Event_Combat_Boss_Interrupted, Payload);
	}

	UE_LOG(LogDFAI, Log, TEXT("[Combat] Boss cast interrupted on %s"), *GetNameSafe(Target));
	return true;
}
