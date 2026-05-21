// Source/DungeonForged/Private/Combat/AN/AN_ComboWindowOpen.cpp
#include "Combat/AN/AN_ComboWindowOpen.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Combat/UDFComboComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "GameFramework/Actor.h"

void UAN_ComboWindowOpen::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	(void)Animation;
	(void)EventReference;
	if (!MeshComp)
	{
		return;
	}
	AActor* const Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}
	if (UDFComboComponent* C = Owner->FindComponentByClass<UDFComboComponent>())
	{
		UAnimMontage* MontageContext = nullptr;
		if (UAnimInstance* const AnimInst = MeshComp->GetAnimInstance())
		{
			MontageContext = AnimInst->GetCurrentActiveMontage();
		}
		C->AdvanceCombo(TEXT("AN_ComboWindowOpen"), MontageContext);
	}
}
