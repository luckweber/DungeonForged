// Source/DungeonForged/Private/GAS/Abilities/UDFAbility_Enemy_Death.cpp
#include "GAS/Abilities/UDFAbility_Enemy_Death.h"

#include "Animation/AnimMontage.h"
#include "Characters/ADFEnemyBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GAS/DFGameplayTags.h"

UUDFAbility_Enemy_Death::UUDFAbility_Enemy_Death()
{
	// Run the post-death pipeline at montage BlendOut start (not OnCompleted) so LockDeathPoseOnMesh
	// fires while the montage is still playing — guarantees Montage_SetPosition() snaps to the final
	// frame before the AnimBP can blend back to idle. Required to prevent the corpse from standing up.
	bFinishDeathOnMontageBlendOut = true;
}

void UUDFAbility_Enemy_Death::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		if (FDFGameplayTags::Ability_Death_Enemy.IsValid())
		{
			AbilityTags.AddTag(FDFGameplayTags::Ability_Death_Enemy);
		}
		if (FDFGameplayTags::Ability_Death.IsValid())
		{
			AbilityTags.AddTag(FDFGameplayTags::Ability_Death);
		}
		if (FDFGameplayTags::Ability_Parent.IsValid())
		{
			CancelAbilitiesWithTag.AddTag(FDFGameplayTags::Ability_Parent);
		}
		if (FDFGameplayTags::Event_Death_Loot.IsValid())
		{
			DeathEventTag = FDFGameplayTags::Event_Death_Loot;
		}
	}
}

UAnimMontage* UUDFAbility_Enemy_Death::ResolveDeathMontage() const
{
	if (const ADFEnemyBase* const Enemy = Cast<ADFEnemyBase>(GetAvatarActorFromActorInfo()))
	{
		if (Enemy->DeathMontage)
		{
			return Enemy->DeathMontage;
		}
	}
	return Super::ResolveDeathMontage();
}

void UUDFAbility_Enemy_Death::ApplyDeathState()
{
	if (ADFEnemyBase* const Enemy = Cast<ADFEnemyBase>(GetAvatarActorFromActorInfo()))
	{
		Enemy->CancelAbilitiesForDeath(this);
		Enemy->ExecuteEnemyDeathPresentationCue();
	}
}

void UUDFAbility_Enemy_Death::OnDeathFlowStarted()
{
	if (ADFEnemyBase* const Enemy = Cast<ADFEnemyBase>(GetAvatarActorFromActorInfo()))
	{
		Enemy->ApplyDeathCorpseMovementState();
		Enemy->DisableEnemyActions();
		if (Enemy->HealthBar)
		{
			Enemy->HealthBar->SetVisibility(false);
		}
		if (Enemy->HasAuthority())
		{
			Enemy->Multicast_PlayDeathCosmetic();
		}
	}
}

void UUDFAbility_Enemy_Death::OnDeathGameplayEvent(FGameplayEventData Payload)
{
	Super::OnDeathGameplayEvent(Payload);
	if (ADFEnemyBase* const Enemy = Cast<ADFEnemyBase>(GetAvatarActorFromActorInfo()))
	{
		Enemy->SpawnDeathLoot();
	}
}

void UUDFAbility_Enemy_Death::OnDeathMontagePipelineFinished(const bool bInterrupted)
{
	(void)bInterrupted;
	ADFEnemyBase* const Enemy = Cast<ADFEnemyBase>(GetAvatarActorFromActorInfo());
	if (!Enemy || !Enemy->HasAuthority())
	{
		return;
	}
	Enemy->SpawnDeathLoot();

	Enemy->Multicast_FinalizeDeathPresentation();
	Enemy->ApplyDeathGameplayState();
	Enemy->MarkDied();
	Enemy->ClearDeathDestroyBackup();

	if (UCapsuleComponent* const Cap = Enemy->GetCapsuleComponent())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	const float DestroyDelayOverride = EnemyDestroyDelay > 0.f ? EnemyDestroyDelay : -1.f;
	Enemy->BeginPostDeathCleanup(DestroyDelayOverride);
}
