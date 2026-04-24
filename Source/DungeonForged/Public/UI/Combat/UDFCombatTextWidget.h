// Source/DungeonForged/Public/UI/Combat/UDFCombatTextWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/Combat/DFCombatTextTypes.h"
#include "Blueprint/UserWidget.h"
#include "UDFCombatTextWidget.generated.h"

class UTextBlock;
class UWidgetAnimation;

class UDFCombatTextSubsystem;

/**
 * One pooled floatie: set text, style, follow world pos in screen space, then return to UDFCombatTextSubsystem.
 * WBP: name root text `DamageText`; optional UMG animation `FloatAnimation` (up + fade, ~1.2s); if absent, 1.2s timer.
 */
UCLASS()
class DUNGEONFORGED_API UDFCombatTextWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Binds a child TextBlock. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "DF|CombatText")
	TObjectPtr<UTextBlock> DamageText = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetAnimOptional), Category = "DF|CombatText")
	TObjectPtr<UWidgetAnimation> FloatAnimation = nullptr;

	/** Pooled: style, scatter, play float anim or duration timer, then return to pool. */
	void InitializeCombatText(
		const FString& InText,
		ECombatTextType Type,
		FVector InWorldLocation,
		float InDuration = 1.2f,
		UDFCombatTextSubsystem* InOwner = nullptr);

	void ReturnToPool();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	friend UDFCombatTextSubsystem;

	/** Filled by subsystem so ReturnToPool works. */
	TWeakObjectPtr<UDFCombatTextSubsystem> OwnerSubsystem;

	FVector WorldLocation = FVector::ZeroVector;
	bool bInUse = false;
	float ScreenScatterX = 0.f;

	FTimerHandle EndTimer;
	FTimerHandle FollowTimer;

	void OnFloatTimeElapsed();
	void UpdateScreenPosition();
	void ApplyStyleForType(ECombatTextType Type) const;
};
