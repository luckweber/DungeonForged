// Source/DungeonForged/Private/Boss/ADFBossTriggerVolume.cpp
#include "Boss/ADFBossTriggerVolume.h"
#include "Boss/ADFBossBase.h"
#include "GAS/DFGameplayTags.h"
#include "UI/UDFBossHealthBarWidget.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "MovieSceneSequencePlayer.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "TimerManager.h"
#include "GameplayEffectTypes.h"

ADFBossTriggerVolume::ADFBossTriggerVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	SetRootComponent(Box);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Box->SetGenerateOverlapEvents(true);
	Box->OnComponentBeginOverlap.AddDynamic(this, &ADFBossTriggerVolume::OnBoxBeginOverlap);
}

void ADFBossTriggerVolume::OnBoxBeginOverlap(
	UPrimitiveComponent* const /*OverlappedComponent*/,
	AActor* const OtherActor,
	UPrimitiveComponent* const /*OtherComp*/,
	const int32 /*OtherBodyIndex*/,
	const bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	if (!HasAuthority())
	{
		return;
	}
	if (bTriggered)
	{
		return;
	}
	if (!OtherActor || !OtherActor->IsA<APawn>())
	{
		return;
	}
	APawn* const P = Cast<APawn>(OtherActor);
	if (!P || !P->IsPlayerControlled())
	{
		return;
	}
	bTriggered = true;
	Box->SetGenerateOverlapEvents(false);

	for (AActor* const A : DoorLockTargets)
	{
		if (!A)
		{
			continue;
		}
		if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(A))
		{
			continue;
		}
		if (FDFGameplayTags::Event_Boss_DoorLock.IsValid())
		{
			FGameplayEventData Pld;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(A, FDFGameplayTags::Event_Boss_DoorLock, Pld);
		}
	}

	Multicast_BeginIntroCinematic();

	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().SetTimer(IntroServerTimer, this, &ADFBossTriggerVolume::OnIntroEnd_Server, IntroEndDelay, false);
	}
}

void ADFBossTriggerVolume::Multicast_BeginIntroCinematic_Implementation()
{
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	const bool bCinematic = IntroLevelSequence != nullptr;
	for (FConstPlayerControllerIterator I = W->GetPlayerControllerIterator(); I; ++I)
	{
		APlayerController* const PC = I->Get();
		if (!PC || !PC->IsLocalController())
		{
			continue;
		}
		if (bCinematic)
		{
			PC->SetCinematicMode(true, true, true, true, true);
			if (APawn* const Pwn = PC->GetPawn())
			{
				if (UAbilitySystemComponent* const Acs = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pwn))
				{
					if (FDFGameplayTags::UI_CinematicLock.IsValid())
					{
						Acs->AddLooseGameplayTag(FDFGameplayTags::UI_CinematicLock);
					}
				}
			}
		}
		if (BossBarWidgetClass && IsValid(TargetBoss))
		{
			if (UDFBossHealthBarWidget* const Wdg = CreateWidget<UDFBossHealthBarWidget>(PC, BossBarWidgetClass))
			{
				Wdg->ShowForBoss(TargetBoss, TargetBoss->GetBossDisplayName());
				Wdg->AddToViewport(100);
			}
		}
	}

	if (IntroLevelSequence)
	{
		FMovieSceneSequencePlaybackSettings Settings;
		ALevelSequenceActor* LSA = nullptr;
		if (ULevelSequencePlayer* const LSP = ULevelSequencePlayer::CreateLevelSequencePlayer(W, IntroLevelSequence, Settings, LSA))
		{
			LSP->OnFinished.AddDynamic(this, &ADFBossTriggerVolume::OnLocalSequenceFinished);
			LSP->Play();
		}
	}
}

void ADFBossTriggerVolume::OnLocalSequenceFinished()
{
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	for (FConstPlayerControllerIterator I = W->GetPlayerControllerIterator(); I; ++I)
	{
		APlayerController* const PC = I->Get();
		if (!PC || !PC->IsLocalController())
		{
			continue;
		}
		PC->SetCinematicMode(false, false, false, true, true);
		if (APawn* const Pwn = PC->GetPawn())
		{
			if (UAbilitySystemComponent* const Acs = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pwn))
			{
				if (FDFGameplayTags::UI_CinematicLock.IsValid())
				{
					Acs->RemoveLooseGameplayTag(FDFGameplayTags::UI_CinematicLock);
				}
			}
		}
	}
}

void ADFBossTriggerVolume::OnIntroEnd_Server()
{
	if (!TargetBoss)
	{
		return;
	}
	if (FDFGameplayTags::Event_Boss_IntroComplete.IsValid() && UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetBoss))
	{
		FGameplayEventData Pld;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			TargetBoss, FDFGameplayTags::Event_Boss_IntroComplete, Pld);
	}
	if (AAIController* const AI = Cast<AAIController>(TargetBoss->GetController()))
	{
		if (UBehaviorTree* const BT = TargetBoss->GetAIBehaviorTreeAsset())
		{
			AI->RunBehaviorTree(BT);
		}
	}
}
