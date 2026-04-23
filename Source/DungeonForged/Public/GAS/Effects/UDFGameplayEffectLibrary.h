// Source/DungeonForged/Public/GAS/Effects/UDFGameplayEffectLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UDFGameplayEffectLibrary.generated.h"

class AActor;
class UGameplayEffect;

UCLASS()
class DUNGEONFORGED_API UDFGameplayEffectLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Outgoing spec: SetByCaller Data.Damage. Picks UGE from DamageTypeTag (True / Magic / Physical default). Requires Instigator with an ASC. */
	UFUNCTION(BlueprintCallable, Category = "GAS|DF", meta = (WorldContext = "Instigator"))
	static FGameplayEffectSpecHandle MakeDamageEffect(float BaseDamage, FGameplayTag DamageTypeTag, AActor* Instigator);

	/** Outgoing spec: SetByCaller Data.Healing. */
	UFUNCTION(BlueprintCallable, Category = "GAS|DF", meta = (WorldContext = "Instigator"))
	static FGameplayEffectSpecHandle MakeHealEffect(float Amount, AActor* Instigator);

	UFUNCTION(BlueprintCallable, Category = "GAS|DF")
	static FActiveGameplayEffectHandle ApplyEffectToSelf(AActor* Target, TSubclassOf<UGameplayEffect> GEClass, float Level = 1.f);

	UFUNCTION(BlueprintCallable, Category = "GAS|DF")
	static FActiveGameplayEffectHandle ApplyEffectToTarget(AActor* Source, AActor* Target, TSubclassOf<UGameplayEffect> GEClass);
};
