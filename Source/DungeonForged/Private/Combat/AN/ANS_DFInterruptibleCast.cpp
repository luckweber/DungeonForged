// Source/DungeonForged/Private/Combat/AN/ANS_DFInterruptibleCast.cpp
#include "Combat/AN/ANS_DFInterruptibleCast.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/DFGameplayTags.h"

UANS_DFInterruptibleCast::UANS_DFInterruptibleCast()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(180, 60, 255);
#endif
}

FString UANS_DFInterruptibleCast::GetNotifyName_Implementation() const
{
	return TEXT("DF Interruptible Cast");
}

void UANS_DFInterruptibleCast::NotifyBegin(USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation, const float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (!MeshComp)
	{
		return;
	}
	if (UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner()))
	{
		if (FDFGameplayTags::State_Combat_Casting_Interruptible.IsValid())
		{
			ASC->AddLooseGameplayTag(FDFGameplayTags::State_Combat_Casting_Interruptible);
		}
	}
}

void UANS_DFInterruptibleCast::NotifyEnd(USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (!MeshComp)
	{
		return;
	}
	if (UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner()))
	{
		if (FDFGameplayTags::State_Combat_Casting_Interruptible.IsValid())
		{
			ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Combat_Casting_Interruptible);
		}
	}
}
