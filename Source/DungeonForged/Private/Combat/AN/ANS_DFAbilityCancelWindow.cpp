// Source/DungeonForged/Private/Combat/AN/ANS_DFAbilityCancelWindow.cpp
#include "Combat/AN/ANS_DFAbilityCancelWindow.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Combat/UDFComboComponent.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GameFramework/Actor.h"

UANS_DFAbilityCancelWindow::UANS_DFAbilityCancelWindow()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(90, 140, 255);
#endif
	if (FDFGameplayTags::Ability_Movement_Dodge.IsValid())
	{
		AllowedCancelTags.AddTag(FDFGameplayTags::Ability_Movement_Dodge);
	}
	if (FDFGameplayTags::Ability_Movement_Jump.IsValid())
	{
		AllowedCancelTags.AddTag(FDFGameplayTags::Ability_Movement_Jump);
	}
	if (FDFGameplayTags::Ability_Movement_AirDash.IsValid())
	{
		AllowedCancelTags.AddTag(FDFGameplayTags::Ability_Movement_AirDash);
	}
}

FString UANS_DFAbilityCancelWindow::GetNotifyName_Implementation() const
{
	return TEXT("DF Ability Cancel Window");
}

void UANS_DFAbilityCancelWindow::NotifyBegin(USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation, const float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	AActor* const Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}
	if (UDFComboComponent* const Combo = Owner->FindComponentByClass<UDFComboComponent>())
	{
		Combo->SetAbilityCancelWindow(AllowedCancelTags);
	}
	if (UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		if (FDFGameplayTags::State_Combat_AbilityCancelWindow_Open.IsValid())
		{
			ASC->AddLooseGameplayTag(FDFGameplayTags::State_Combat_AbilityCancelWindow_Open);
		}
	}
}

void UANS_DFAbilityCancelWindow::NotifyEnd(USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	AActor* const Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}
	if (UDFComboComponent* const Combo = Owner->FindComponentByClass<UDFComboComponent>())
	{
		Combo->ClearAbilityCancelWindow();
	}
	if (UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		if (FDFGameplayTags::State_Combat_AbilityCancelWindow_Open.IsValid())
		{
			ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Combat_AbilityCancelWindow_Open);
		}
	}
}
