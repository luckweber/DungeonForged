// Source/DungeonForged/Private/GAS/Abilities/Boss/DFBossAbilityCommons.cpp
#include "GAS/Abilities/Boss/DFBossAbilityCommons.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Debuff_Stun.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Kismet/GameplayStatics.h"

ACharacter* DFBossAbilityCommons::GetFirstPlayerCharacter(UWorld* const World, AActor* const Ignore)
{
	if (!World)
	{
		return nullptr;
	}
	if (APlayerController* const PC = UGameplayStatics::GetPlayerController(World, 0))
	{
		if (ACharacter* const C = Cast<ACharacter>(PC->GetPawn()))
		{
			if (C != Ignore)
			{
				return C;
			}
		}
	}
	return nullptr;
}

void DFBossAbilityCommons::ApplySetCallerCooldown(
	UAbilitySystemComponent* const ASC, const TSubclassOf<UGameplayEffect> CooldownClass, const float Seconds)
{
	if (!ASC || !CooldownClass || Seconds <= 0.f)
	{
		return;
	}
	const FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
	const FGameplayEffectSpecHandle S = ASC->MakeOutgoingSpec(CooldownClass, 1.f, Ctx);
	if (S.IsValid() && S.Data.IsValid() && FDFGameplayTags::Data_Cooldown.IsValid())
	{
		S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Cooldown, Seconds);
		ASC->ApplyGameplayEffectSpecToSelf(*S.Data.Get());
	}
}

void DFBossAbilityCommons::StunTarget(
	UAbilitySystemComponent* const SourceOwnerASC, AActor* const Target, const float Duration)
{
	if (!SourceOwnerASC || !Target)
	{
		return;
	}
	UAbilitySystemComponent* const Tgt = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!Tgt)
	{
		return;
	}
	const FGameplayEffectContextHandle Ctx = SourceOwnerASC->MakeEffectContext();
	const FGameplayEffectSpecHandle S = SourceOwnerASC->MakeOutgoingSpec(UGE_Debuff_Stun::StaticClass(), 1.f, Ctx);
	if (S.IsValid() && S.Data.IsValid() && FDFGameplayTags::Data_Duration.IsValid())
	{
		S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, Duration);
		SourceOwnerASC->ApplyGameplayEffectSpecToTarget(*S.Data.Get(), Tgt);
	}
}
