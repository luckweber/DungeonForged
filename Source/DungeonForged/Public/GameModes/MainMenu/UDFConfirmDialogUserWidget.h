// Source/DungeonForged/Public/GameModes/MainMenu/UDFConfirmDialogUserWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "Blueprint/UserWidget.h"
#include "UDFConfirmDialogUserWidget.generated.h"

class UTextBlock;
class UButton;
class UImage;
class UWidgetAnimation;

UCLASS(Blueprintable)
class DUNGEONFORGED_API UDFConfirmDialogUserWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	/** C++: bind @c FSimpleDelegate (Blueprint uses child widget graph). */
	void ShowDialog(FText const& Title, FText const& Body, FSimpleDelegate InOnConfirm);

	/** Blueprint-only hook for open scale (0.8→1) when @c bPlayOpenAnim is true. */
	UFUNCTION(BlueprintNativeEvent, Category = "DF|UI|Dialog")
	void PlayOpenPresentation();
	virtual void PlayOpenPresentation_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "DF|UI|Dialog")
	void PlayClosePresentation();
	virtual void PlayClosePresentation_Implementation();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void OnConfirmClicked();
	UFUNCTION()
	void OnCancelClicked();

	FSimpleDelegate OnConfirmDelegate;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UImage> DarkOverlay = nullptr;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText = nullptr;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> BodyText = nullptr;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UButton> ConfirmButton = nullptr;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UButton> CancelButton = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim), BlueprintReadOnly, Category = "DF|UI|Dialog")
	TObjectPtr<UWidgetAnimation> OpenAnim = nullptr;
};
