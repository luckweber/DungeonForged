// Source/DungeonForged/Public/UI/UDFUserWidgetBase.h
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayEffectTypes.h"
#include "Blueprint/UserWidget.h"
#include "UDFUserWidgetBase.generated.h"

class UAbilitySystemComponent;
class ADFPlayerCharacter;
class ADFPlayerState;

struct FDFAttributeChangeBinding
{
	TWeakObjectPtr<UAbilitySystemComponent> ASC;
	FGameplayAttribute Attribute;
	FDelegateHandle Handle;
};

UCLASS(Abstract, Blueprintable, BlueprintType)
class DUNGEONFORGED_API UDFUserWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DF|UI|Player")
	ADFPlayerCharacter* GetDFPlayerCharacter() const;

	UFUNCTION(BlueprintCallable, Category = "DF|UI|Player")
	ADFPlayerState* GetDFPlayerState() const;

	UFUNCTION(BlueprintCallable, Category = "DF|UI|GAS")
	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	/** Tracked in AttributeChangeBindings; UnbindAllAttributeChanges runs in NativeDestruct. */
	FDelegateHandle BindToAttributeChanges(
		UAbilitySystemComponent* InASC,
		const FGameplayAttribute& InAttribute,
		TDelegate<void(const FOnAttributeChangeData&)> InCallback);

	void UnbindAllAttributeChanges();

protected:
	virtual void NativeDestruct() override;

	TArray<FDFAttributeChangeBinding> AttributeChangeBindings;
};
