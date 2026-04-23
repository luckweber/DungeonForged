// Source/DungeonForged/Private/GAS/Effects/UDFExec_FortitudeWounded.cpp
#include "GAS/Effects/UDFExec_FortitudeWounded.h"
#include "GAS/Effects/UGE_Passive_Buff_BattleFury.h"
#include "GAS/UDFAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

void UDFExec_FortitudeWounded::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	(void)OutExecutionOutput;
	UAbilitySystemComponent* const T = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!T)
	{
		return;
	}
	const float M = T->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
	const float H = T->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute());
	if (M <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	if (H / M >= 0.5f)
	{
		return;
	}
	const FGameplayEffectContextHandle& Ctx = ExecutionParams.GetOwningSpec().GetContext();
	const FGameplayEffectSpecHandle S = T->MakeOutgoingSpec(UGE_Passive_Buff_BattleFury::StaticClass(), 1.f, Ctx);
	if (S.IsValid() && S.Data)
	{
		T->ApplyGameplayEffectSpecToSelf(*S.Data);
	}
}
