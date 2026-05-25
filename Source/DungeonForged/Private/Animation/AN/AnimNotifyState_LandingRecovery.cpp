// Source/DungeonForged/Private/Animation/AN/AnimNotifyState_LandingRecovery.cpp
#include "Animation/AN/AnimNotifyState_LandingRecovery.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/UDFAnimInstance.h"
#include "Combat/DFJumpDebug.h"
#include "GAS/DFGameplayTags.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_LandingRecovery::NotifyBegin(USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation, const float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (UAbilitySystemComponent* const ASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp ? MeshComp->GetOwner() : nullptr))
	{
		if (FDFGameplayTags::State_Landing.IsValid() && !ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Landing))
		{
			ASC->AddLooseGameplayTag(FDFGameplayTags::State_Landing);
		}
	}
	if (UUDFAnimInstance* const Anim = MeshComp ? Cast<UUDFAnimInstance>(MeshComp->GetAnimInstance()) : nullptr)
	{
		Anim->NotifyLandingRecoveryBegin(TotalDuration);
	}
	DFJumpDebug::Log(TEXT("Landing recovery begin"));
}

void UAnimNotifyState_LandingRecovery::NotifyEnd(USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (UAbilitySystemComponent* const ASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp ? MeshComp->GetOwner() : nullptr))
	{
		if (FDFGameplayTags::State_Landing.IsValid())
		{
			ASC->SetLooseGameplayTagCount(FDFGameplayTags::State_Landing, 0);
		}
	}
	if (UUDFAnimInstance* const Anim = MeshComp ? Cast<UUDFAnimInstance>(MeshComp->GetAnimInstance()) : nullptr)
	{
		Anim->NotifyLandingRecoveryEnd();
	}
	DFJumpDebug::Log(TEXT("Landing recovery end"));
}
