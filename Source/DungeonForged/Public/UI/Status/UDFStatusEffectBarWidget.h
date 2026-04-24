// Source/DungeonForged/Public/UI/Status/UDFStatusEffectBarWidget.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFStatusEffectBarWidget.generated.h"

class UAbilitySystemComponent;
class UDFStatusEffectIconWidget;
class UDFStatusLibrary;
class UHorizontalBox;
class UWidgetAnimation;
class UDFStatusEffectTooltipWidget;

UCLASS(Abstract, Blueprintable)
class DUNGEONFORGED_API UDFStatusEffectBarWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

	friend class UDFStatusEffectIconWidget;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|UI|Status")
	TObjectPtr<UDFStatusLibrary> StatusLibrary = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|UI|Status")
	TSubclassOf<UDFStatusEffectIconWidget> StatusIconWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|UI|Status|Tooltip")
	TSubclassOf<UDFStatusEffectTooltipWidget> TooltipWidgetClass;

	void ReleaseIconToPool(UDFStatusEffectIconWidget* Icon);

	static FGameplayTag ChooseDisplayTag(const FGameplayEffectSpec& Spec, const UDFStatusLibrary* Lib);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void BindToAsc(UAbilitySystemComponent* ASC);
	void UnbindFromAsc();

	void OnEffectAdded(UAbilitySystemComponent* Target, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);
	void OnEffectRemoved(const FActiveGameplayEffect& Removed);

	void RefreshExistingActiveEffects();
	void SortAndLayoutIcons();
	void RemoveIconForHandle(FActiveGameplayEffectHandle Handle);

	UDFStatusEffectIconWidget* AcquireIcon();
	void ReparentIconToRow(UDFStatusEffectIconWidget* Icon, bool bDebuffRow);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> BuffRow = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> DebuffRow = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> IconAddBounceAnim = nullptr;

	TWeakObjectPtr<UAbilitySystemComponent> BoundAsc;
	FDelegateHandle AddedHandle;
	FDelegateHandle RemovedHandle;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDFStatusEffectIconWidget>> IconPool;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDFStatusEffectIconWidget>> ActiveIcons;

	TMap<FActiveGameplayEffectHandle, TObjectPtr<UDFStatusEffectIconWidget>> IconByHandle;
};
