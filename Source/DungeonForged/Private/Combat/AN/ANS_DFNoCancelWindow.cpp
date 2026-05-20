// Source/DungeonForged/Private/Combat/AN/ANS_DFNoCancelWindow.cpp
#include "Combat/AN/ANS_DFNoCancelWindow.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/DFGameplayTags.h"

UANS_DFNoCancelWindow::UANS_DFNoCancelWindow()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(180, 40, 40);
#endif
}

FString UANS_DFNoCancelWindow::GetNotifyName_Implementation() const
{
	return TEXT("DF No Cancel Frames");
}

void UANS_DFNoCancelWindow::NotifyBegin(USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation, const float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	AActor* const Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!ASC || !FDFGameplayTags::State_Combat_NoCancelFrames.IsValid())
	{
		return;
	}
	ASC->AddLooseGameplayTag(FDFGameplayTags::State_Combat_NoCancelFrames);
}

void UANS_DFNoCancelWindow::NotifyEnd(USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	AActor* const Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!ASC || !FDFGameplayTags::State_Combat_NoCancelFrames.IsValid())
	{
		return;
	}
	ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Combat_NoCancelFrames);
}
