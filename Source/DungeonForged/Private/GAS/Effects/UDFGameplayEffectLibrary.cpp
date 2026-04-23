// Source/DungeonForged/Private/GAS/Effects/UDFGameplayEffectLibrary.cpp
#include "GAS/Effects/UDFGameplayEffectLibrary.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Damage_Magic.h"
#include "GAS/Effects/UGE_Damage_Physical.h"
#include "GAS/Effects/UGE_Damage_True.h"
#include "GAS/Effects/UGE_Heal_Instant.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"

static TSubclassOf<UGameplayEffect> GEPickDamageClass(const FGameplayTag& DamageTypeTag)
{
	if (FDFGameplayTags::Effect_Damage_True.IsValid() && DamageTypeTag.MatchesTag(FDFGameplayTags::Effect_Damage_True))
	{
		return UGE_Damage_True::StaticClass();
	}
	if (FDFGameplayTags::Effect_Damage_Magic.IsValid() && DamageTypeTag.MatchesTag(FDFGameplayTags::Effect_Damage_Magic))
	{
		return UGE_Damage_Magic::StaticClass();
	}
	if (FDFGameplayTags::Effect_Damage_Physical.IsValid() && DamageTypeTag.MatchesTag(FDFGameplayTags::Effect_Damage_Physical))
	{
		return UGE_Damage_Physical::StaticClass();
	}
	return UGE_Damage_Physical::StaticClass();
}

FGameplayEffectSpecHandle UDFGameplayEffectLibrary::MakeDamageEffect(
	const float BaseDamage, const FGameplayTag DamageTypeTag, AActor* const Instigator)
{
	UAbilitySystemComponent* const ASC = Instigator
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator)
		: nullptr;
	if (!ASC)
	{
		return FGameplayEffectSpecHandle();
	}
	const TSubclassOf<UGameplayEffect> GeClass = GEPickDamageClass(DamageTypeTag);
	if (!GeClass)
	{
		return FGameplayEffectSpecHandle();
	}
	FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
	Ctx.AddInstigator(Instigator, Instigator);
	if (Instigator)
	{
		Ctx.AddSourceObject(Instigator);
	}
	FGameplayEffectSpecHandle SpecH = ASC->MakeOutgoingSpec(GeClass, 1.f, Ctx);
	if (FGameplayEffectSpec* const Spec = SpecH.Data.Get())
	{
		const FGameplayTag Dmg = FDFGameplayTags::Data_Damage.IsValid()
			? FDFGameplayTags::Data_Damage
			: FGameplayTag::RequestGameplayTag(FName("Data.Damage"), false);
		if (Dmg.IsValid())
		{
			Spec->SetSetByCallerMagnitude(Dmg, BaseDamage);
		}
	}
	return SpecH;
}

FGameplayEffectSpecHandle UDFGameplayEffectLibrary::MakeHealEffect(const float Amount, AActor* const Instigator)
{
	UAbilitySystemComponent* const ASC = Instigator
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator)
		: nullptr;
	if (!ASC)
	{
		return FGameplayEffectSpecHandle();
	}
	FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
	Ctx.AddInstigator(Instigator, Instigator);
	if (Instigator)
	{
		Ctx.AddSourceObject(Instigator);
	}
	FGameplayEffectSpecHandle SpecH = ASC->MakeOutgoingSpec(UGE_Heal_Instant::StaticClass(), 1.f, Ctx);
	if (FGameplayEffectSpec* const Spec = SpecH.Data.Get())
	{
		const FGameplayTag H = FDFGameplayTags::Data_Healing.IsValid()
			? FDFGameplayTags::Data_Healing
			: FGameplayTag::RequestGameplayTag(FName("Data.Healing"), false);
		if (H.IsValid())
		{
			Spec->SetSetByCallerMagnitude(H, Amount);
		}
	}
	return SpecH;
}

FActiveGameplayEffectHandle UDFGameplayEffectLibrary::ApplyEffectToSelf(AActor* const Target, const TSubclassOf<UGameplayEffect> GEClass, const float Level)
{
	UAbilitySystemComponent* const ASC = Target
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target)
		: nullptr;
	if (!ASC || !GEClass)
	{
		return FActiveGameplayEffectHandle();
	}
	UGameplayEffect* const Def = Cast<UGameplayEffect>(GEClass->GetDefaultObject());
	if (!Def)
	{
		return FActiveGameplayEffectHandle();
	}
	FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
	if (Target)
	{
		Ctx.AddInstigator(Target, Target);
	}
	return ASC->ApplyGameplayEffectToSelf(Def, Level, Ctx);
}

FActiveGameplayEffectHandle UDFGameplayEffectLibrary::ApplyEffectToTarget(AActor* const Source, AActor* const Target, const TSubclassOf<UGameplayEffect> GEClass)
{
	UAbilitySystemComponent* const Src = Source
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Source)
		: nullptr;
	UAbilitySystemComponent* const Tgt = Target
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target)
		: nullptr;
	if (!Src || !Tgt || !GEClass)
	{
		return FActiveGameplayEffectHandle();
	}
	UGameplayEffect* const Def = Cast<UGameplayEffect>(GEClass->GetDefaultObject());
	if (!Def)
	{
		return FActiveGameplayEffectHandle();
	}
	FGameplayEffectContextHandle Ctx = Src->MakeEffectContext();
	if (Source)
	{
		Ctx.AddInstigator(Source, Source);
		Ctx.AddSourceObject(Source);
	}
	return Src->ApplyGameplayEffectToTarget(Def, Tgt, 1.f, Ctx);
}
