// Source/DungeonForged/Private/GAS/UDFTrueDamageExecution.cpp
#include "GAS/UDFTrueDamageExecution.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Elemental/UDFElementalLibrary.h"
#include "GAS/Elemental/UDFElementalReactionSubsystem.h"
#include "GAS/UDFAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameplayEffectTypes.h"

UDFTrueDamageExecution::UDFTrueDamageExecution()
	: UGameplayEffectExecutionCalculation()
{
}

void UDFTrueDamageExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const UAbilitySystemComponent* const TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const FGameplayTag DataDamageTag = FDFGameplayTags::Data_Damage.IsValid()
		? FDFGameplayTags::Data_Damage
		: FGameplayTag::RequestGameplayTag(FName("Data.Damage"), false);
	float Base = DataDamageTag.IsValid()
		? Spec.GetSetByCallerMagnitude(DataDamageTag, false, 0.f)
		: 0.f;

	const EDFElementType AttackElement = UDFElementalLibrary::ResolveElementFromEffectSpec(Spec);
	if (AttackElement != EDFElementType::None && AttackElement != EDFElementType::ElementTrue)
	{
		AActor* TargetActor = TargetASC->GetAvatarActor();
		AActor* Instigator = Spec.GetEffectContext().GetInstigator();
		if (const UAbilitySystemComponent* const SourceASC = ExecutionParams.GetSourceAbilitySystemComponent())
		{
			if (!Instigator)
			{
				Instigator = SourceASC->GetAvatarActor();
			}
		}
		if (UWorld* const World = TargetASC->GetWorld())
		{
			if (UDFElementalReactionSubsystem* const ElemSub = World->GetSubsystem<UDFElementalReactionSubsystem>())
			{
				Base = ElemSub->ScaleBaseDamageWithElement(Base, AttackElement, TargetActor, Instigator);
			}
		}
	}

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UDFAttributeSet::GetIncomingDamageAttribute(),
		EGameplayModOp::Additive, FMath::Max(0.f, Base)));
}
