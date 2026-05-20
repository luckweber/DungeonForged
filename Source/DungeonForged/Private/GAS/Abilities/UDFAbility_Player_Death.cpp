// Source/DungeonForged/Private/GAS/Abilities/UDFAbility_Player_Death.cpp
#include "GAS/Abilities/UDFAbility_Player_Death.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Characters/ADFPlayerCharacter.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Death.h"

UUDFAbility_Player_Death::UUDFAbility_Player_Death()
{
}

void UUDFAbility_Player_Death::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		if (FDFGameplayTags::Ability_Death_Player.IsValid())
		{
			AbilityTags.AddTag(FDFGameplayTags::Ability_Death_Player);
		}
		if (FDFGameplayTags::Ability_Death.IsValid())
		{
			AbilityTags.AddTag(FDFGameplayTags::Ability_Death);
		}
		if (FDFGameplayTags::Ability_Parent.IsValid())
		{
			CancelAbilitiesWithTag.AddTag(FDFGameplayTags::Ability_Parent);
		}
	}
}

UAnimMontage* UUDFAbility_Player_Death::ResolveDeathMontage() const
{
	if (const ADFPlayerCharacter* const Player = Cast<ADFPlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (Player->DeathMontage)
		{
			return Player->DeathMontage;
		}
	}
	return Super::ResolveDeathMontage();
}

void UUDFAbility_Player_Death::ApplyDeathState()
{
	if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (ASC->GetOwner() && ASC->GetOwner()->HasAuthority())
		{
			FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
			if (AActor* const Avatar = GetAvatarActorFromActorInfo())
			{
				Ctx.AddSourceObject(Avatar);
			}
			const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGE_Death::StaticClass(), 1.f, Ctx);
			if (Spec.IsValid() && Spec.Data.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
				return;
			}
		}
		if (FDFGameplayTags::State_Dead.IsValid())
		{
			ASC->AddLooseGameplayTag(FDFGameplayTags::State_Dead, 1);
		}
	}
}

void UUDFAbility_Player_Death::OnDeathFlowStarted()
{
	if (ADFPlayerCharacter* const Player = Cast<ADFPlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		Player->BeginDeathPresentationFromAbility();
		if (Player->HasAuthority())
		{
			Player->Multicast_PlayPlayerDeathCinematic();
		}
	}
}

void UUDFAbility_Player_Death::OnDeathMontagePipelineFinished(const bool bInterrupted)
{
	(void)bInterrupted;
	ADFPlayerCharacter* const Player = Cast<ADFPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!Player)
	{
		return;
	}
	if (Player->HasAuthority())
	{
		Player->Multicast_FinalizeDeathPresentation();
	}
	else
	{
		Player->FinalizeDeathPresentation();
	}
}
