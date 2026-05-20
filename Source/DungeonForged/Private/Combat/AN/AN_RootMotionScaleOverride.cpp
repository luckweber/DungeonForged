// Source/DungeonForged/Private/Combat/AN/AN_RootMotionScaleOverride.cpp
#include "Combat/AN/AN_RootMotionScaleOverride.h"

#include "GameFramework/Character.h"

UAN_RootMotionScaleOverride::UAN_RootMotionScaleOverride()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(180, 120, 255);
#endif
}

FString UAN_RootMotionScaleOverride::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("RM Scale %.2f"), TranslationScale);
}

void UAN_RootMotionScaleOverride::Notify(
	USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (!MeshComp)
	{
		return;
	}
	if (ACharacter* const Char = Cast<ACharacter>(MeshComp->GetOwner()))
	{
		Char->SetAnimRootMotionTranslationScale(FMath::Max(0.f, TranslationScale));
	}
}
