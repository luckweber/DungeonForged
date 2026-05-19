// Source/DungeonForged/Public/UI/UDFEnemyHealthBarWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/UDFUserWidgetBase.h"
#include "GAS/UDFAttributeSet.h"
#include "UDFEnemyHealthBarWidget.generated.h"

class UProgressBar;
class UTextBlock;
class ADFEnemyBase;
class UAbilitySystemComponent;

/**
 * Floating HP bar on @c ADFEnemyBase::HealthBar (WidgetComponent).
 * Call @ref SetupObservedEnemy from the enemy after @c InitWidget (done in @c ADFEnemyBase::BeginPlay).
 */
UCLASS(Blueprintable, BlueprintType)
class DUNGEONFORGED_API UDFEnemyHealthBarWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	/** Binds to the enemy ASC (not the local player). */
	void SetupObservedEnemy(ADFEnemyBase* InEnemy, const FText& InDisplayName = FText::GetEmpty());

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void StartRebindTimer();
	void StopRebindTimer();
	void OnRebindTimerTick();
	void TryBindEnemyAttributes();
	void OnHealthAttrChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthAttrChanged(const FOnAttributeChangeData& Data);
	void OnEnemyHealthChanged(float CurrentHealth, float MaxHealth);
	void ResolveWidgetBindings();
	void RefreshHealthFill();
	void ApplyDisplayName();
	void ClearEnemyBindings();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> EnemyHealthBar = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EnemyNameText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthValueText = nullptr;

	/** When true, @c HealthValueText shows current / max HP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|UI|Enemy")
	bool bShowHealthNumbers = false;

	TWeakObjectPtr<ADFEnemyBase> ObservedEnemy;
	TWeakObjectPtr<UAbilitySystemComponent> EnemyAsc;
	TWeakObjectPtr<UDFAttributeSet> BoundAttributeSet;
	FDelegateHandle HealthChangedDelegateHandle;
	FText CachedDisplayName;
	bool bAttributesBound = false;
	FTimerHandle RebindTimerHandle;
	static constexpr float RebindIntervalSec = 0.25f;
};
