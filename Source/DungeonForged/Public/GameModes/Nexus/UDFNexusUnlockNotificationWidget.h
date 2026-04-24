// Source/DungeonForged/Public/GameModes/Nexus/UDFNexusUnlockNotificationWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "UDFNexusUnlockNotificationWidget.generated.h"

class UImage;
class UTextBlock;
class UWidgetAnimation;

UCLASS()
class DUNGEONFORGED_API UDFNexusUnlockNotificationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Nexus|UI")
	void SetUnlockContent(const FText& Title, const FText& Name, UTexture2D* OptionalIcon = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Nexus|UI")
	void PlayShowThenHide(float DisplaySeconds = 4.f);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UImage> UnlockIcon = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetAnim, AllowPrivateAccess = true))
	TObjectPtr<UWidgetAnimation> SlideInAnim = nullptr;

private:
	FTimerHandle AutoHideHandle;
	UFUNCTION()
	void HideSelf();
};
