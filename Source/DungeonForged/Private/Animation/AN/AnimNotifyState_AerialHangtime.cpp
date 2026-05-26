// Source/DungeonForged/Private/Animation/AN/AnimNotifyState_AerialHangtime.cpp
#include "Animation/AN/AnimNotifyState_AerialHangtime.h"

#include "Characters/UDFCharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

void UAnimNotifyState_AerialHangtime::NotifyBegin(USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation, const float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	ACharacter* const Char = MeshComp ? Cast<ACharacter>(MeshComp->GetOwner()) : nullptr;
	UDFCharacterMovementComponent* const CMC = Char ? Cast<UDFCharacterMovementComponent>(Char->GetCharacterMovement()) : nullptr;
	if (!CMC)
	{
		return;
	}
	CMC->AerialHangtimeSavedGravity = CMC->GravityScale;
	CMC->GravityScale = CMC->DFGravityScale * GravityScaleMultiplier;
}

void UAnimNotifyState_AerialHangtime::NotifyEnd(USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	ACharacter* const Char = MeshComp ? Cast<ACharacter>(MeshComp->GetOwner()) : nullptr;
	UDFCharacterMovementComponent* const CMC = Char ? Cast<UDFCharacterMovementComponent>(Char->GetCharacterMovement()) : nullptr;
	if (!CMC)
	{
		return;
	}
	if (CMC->AerialHangtimeSavedGravity >= 0.f)
	{
		CMC->GravityScale = CMC->AerialHangtimeSavedGravity;
		CMC->AerialHangtimeSavedGravity = -1.f;
	}
	else
	{
		CMC->GravityScale = CMC->DFGravityScale;
	}
}
