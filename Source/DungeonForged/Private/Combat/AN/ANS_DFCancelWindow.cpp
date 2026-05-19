// Source/DungeonForged/Private/Combat/AN/ANS_DFCancelWindow.cpp
#include "Combat/AN/ANS_DFCancelWindow.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GameFramework/Actor.h"

namespace
{
UAbilitySystemComponent* GetASC_CancelWnd(USkeletalMeshComponent* const MeshComp)
{
	AActor* const Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
}
} // namespace

UANS_DFCancelWindow::UANS_DFCancelWindow()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(30, 200, 90);
#endif
}

FString UANS_DFCancelWindow::GetNotifyName_Implementation() const
{
	return TEXT("DF Cancel Window");
}

void UANS_DFCancelWindow::NotifyBegin(USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation, const float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	UAbilitySystemComponent* const ASC = GetASC_CancelWnd(MeshComp);
	if (!ASC)
	{
		return;
	}
	if (FDFGameplayTags::State_Combat_CancelWindow_Open.IsValid())
	{
		ASC->AddLooseGameplayTag(FDFGameplayTags::State_Combat_CancelWindow_Open);
	}
	if (bSendGameplayEvents && MeshComp && FDFGameplayTags::Event_Combat_CancelWindow_Open.IsValid())
	{
		AActor* const Owner = MeshComp->GetOwner();
		FGameplayEventData Payload;
		Payload.EventTag = FDFGameplayTags::Event_Combat_CancelWindow_Open;
		Payload.Instigator = Owner;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Owner, FDFGameplayTags::Event_Combat_CancelWindow_Open, Payload);
	}
}

void UANS_DFCancelWindow::NotifyEnd(USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	UAbilitySystemComponent* const ASC = GetASC_CancelWnd(MeshComp);
	if (!ASC)
	{
		return;
	}
	if (FDFGameplayTags::State_Combat_CancelWindow_Open.IsValid())
	{
		ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Combat_CancelWindow_Open);
	}
	if (bSendGameplayEvents && MeshComp && FDFGameplayTags::Event_Combat_CancelWindow_Close.IsValid())
	{
		AActor* const Owner = MeshComp->GetOwner();
		FGameplayEventData Payload;
		Payload.EventTag = FDFGameplayTags::Event_Combat_CancelWindow_Close;
		Payload.Instigator = Owner;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Owner, FDFGameplayTags::Event_Combat_CancelWindow_Close, Payload);
	}
}
