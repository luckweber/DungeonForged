// Source/DungeonForged/Public/GameModes/Nexus/ADFRunPortal.h
#pragma once

#include "CoreMinimal.h"
#include "Interaction/ADFInteractableBase.h"
#include "ADFRunPortal.generated.h"

class UNiagaraComponent;
class UPointLightComponent;
class ACharacter;
class APlayerController;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFRunPortal : public ADFInteractableBase
{
	GENERATED_BODY()

public:
	ADFRunPortal();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nexus|Portal")
	TObjectPtr<UNiagaraComponent> PortalVFX = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nexus|Portal")
	TObjectPtr<UPointLightComponent> PortalLight = nullptr;

	/** Tints by @c APlayerController to reflect meta; optional. */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Portal")
	void SetPortalColorByMetaLevel(int32 MetaLevel);

	virtual void BeginPlay() override;
	virtual void Interact_Implementation(ACharacter* Interactor) override;
};
