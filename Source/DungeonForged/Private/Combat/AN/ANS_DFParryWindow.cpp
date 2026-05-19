// Source/DungeonForged/Private/Combat/AN/ANS_DFParryWindow.cpp
#include "Combat/AN/ANS_DFParryWindow.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GameFramework/Actor.h"

namespace
{
UAbilitySystemComponent* GetASC_Parry(USkeletalMeshComponent* const MeshComp)
{
	AActor* const Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
}
} // namespace

UANS_DFParryWindow::UANS_DFParryWindow()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(255, 215, 0); // gold = parry timing
#endif
}

FString UANS_DFParryWindow::GetNotifyName_Implementation() const
{
	return TEXT("DF Parry Window");
}

void UANS_DFParryWindow::NotifyBegin(USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation, const float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	UAbilitySystemComponent* const ASC = GetASC_Parry(MeshComp);
	if (!ASC)
	{
		return;
	}
	if (FDFGameplayTags::State_Combat_ParryWindow_Open.IsValid())
	{
		ASC->AddLooseGameplayTag(FDFGameplayTags::State_Combat_ParryWindow_Open);
	}
	if (bSendGameplayEvents && MeshComp && FDFGameplayTags::Event_Combat_ParryWindow_Open.IsValid())
	{
		AActor* const Owner = MeshComp->GetOwner();
		FGameplayEventData Payload;
		Payload.EventTag = FDFGameplayTags::Event_Combat_ParryWindow_Open;
		Payload.Instigator = Owner;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Owner, FDFGameplayTags::Event_Combat_ParryWindow_Open, Payload);
	}
}

void UANS_DFParryWindow::NotifyEnd(USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	UAbilitySystemComponent* const ASC = GetASC_Parry(MeshComp);
	if (!ASC)
	{
		return;
	}
	if (FDFGameplayTags::State_Combat_ParryWindow_Open.IsValid())
	{
		ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Combat_ParryWindow_Open);
	}
	if (bSendGameplayEvents && MeshComp && FDFGameplayTags::Event_Combat_ParryWindow_Close.IsValid())
	{
		AActor* const Owner = MeshComp->GetOwner();
		FGameplayEventData Payload;
		Payload.EventTag = FDFGameplayTags::Event_Combat_ParryWindow_Close;
		Payload.Instigator = Owner;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Owner, FDFGameplayTags::Event_Combat_ParryWindow_Close, Payload);
	}
}
