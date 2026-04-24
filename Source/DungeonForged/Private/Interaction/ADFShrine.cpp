// Source/DungeonForged/Private/Interaction/ADFShrine.cpp
#include "Interaction/ADFShrine.h"
#include "Run/DFRunManager.h"
#include "GAS/UDFAttributeSet.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffect.h"

ADFShrine::ADFShrine()
{
	VFXComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("VFXComponent"));
	VFXComponent->SetupAttachment(RootComponent);
}

void ADFShrine::BeginPlay()
{
	Super::BeginPlay();
	if (VFXComponent && ActiveVFX)
	{
		VFXComponent->SetAsset(ActiveVFX);
		VFXComponent->Activate(true);
	}
}

FText ADFShrine::GetInteractionText_Implementation() const
{
	return NSLOCTEXT("DF", "ShrineUse", "Use shrine");
}

bool ADFShrine::CanInteract_Implementation(ACharacter* const Interactor) const
{
	return bIsInteractable && Interactor && !bOnCooldown;
}

void ADFShrine::ApplyMystery(ACharacter* const Interactor)
{
	if (!Interactor)
	{
		return;
	}
	const IAbilitySystemInterface* const IASI = Cast<IAbilitySystemInterface>(Interactor);
	UAbilitySystemComponent* const ASC = IASI ? IASI->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return;
	}
	const int32 Roll = FMath::RandRange(0, 2);
	if (Roll == 0)
	{
		const float M = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
		ASC->ApplyModToAttribute(UDFAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, 0.5f * M);
		return;
	}
	const int32 Ix = Roll - 1;
	if (MysteryEffects.IsValidIndex(Ix) && MysteryEffects[Ix])
	{
		const FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
		const FGameplayEffectSpecHandle S = ASC->MakeOutgoingSpec(MysteryEffects[Ix], 1.f, Ctx);
		if (S.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*S.Data.Get());
		}
	}
}

void ADFShrine::Interact_Implementation(ACharacter* const Interactor)
{
	if (!HasAuthority() || !Interactor)
	{
		return;
	}
	if (GoldRewardMax > 0)
	{
		if (UWorld* const W = GetWorld())
		{
			if (UGameInstance* const GI = W->GetGameInstance())
			{
				if (UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
				{
					const int32 G = FMath::RandRange(
						FMath::Min(GoldRewardMin, GoldRewardMax),
						FMath::Max(GoldRewardMin, GoldRewardMax));
					if (G > 0)
					{
						RM->AddRunGold(G);
					}
				}
			}
		}
	}
	if (ShrineType == EDFShrineType::Mystery)
	{
		ApplyMystery(Interactor);
	}
	else
	{
		if (IAbilitySystemInterface* const IASI = Cast<IAbilitySystemInterface>(Interactor))
		{
			if (UAbilitySystemComponent* const ASC = IASI->GetAbilitySystemComponent())
			{
				if (ShrineEffect)
				{
					const FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
					const FGameplayEffectSpecHandle S = ASC->MakeOutgoingSpec(ShrineEffect, 1.f, Ctx);
					if (S.IsValid())
					{
						ASC->ApplyGameplayEffectSpecToSelf(*S.Data.Get());
					}
				}
			}
		}
	}
	if (VFXComponent)
	{
		if (UsedVFX)
		{
			VFXComponent->SetAsset(UsedVFX);
			VFXComponent->Activate(true);
		}
		else
		{
			VFXComponent->Deactivate();
		}
	}
	if (CooldownBetweenUses > 0.f)
	{
		SetCooldown();
	}
	else if (bSingleUse)
	{
		bIsInteractable = false;
	}
}

void ADFShrine::SetCooldown()
{
	bOnCooldown = true;
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			CooldownTimer, this, &ADFShrine::OnCooldownEnd, CooldownBetweenUses, false);
	}
}

void ADFShrine::OnCooldownEnd()
{
	bOnCooldown = false;
}

void ADFShrine::ClearCooldown()
{
	bOnCooldown = false;
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(CooldownTimer);
	}
}
