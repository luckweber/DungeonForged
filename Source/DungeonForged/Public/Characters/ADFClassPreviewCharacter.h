// Source/DungeonForged/Public/Characters/ADFClassPreviewCharacter.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ADFClassPreviewCharacter.generated.h"

/**
 * Personagem só para UI de seleção de classe (mesh + AnimBP): sem GAS, combate nem modular equip.
 * Prefira um Blueprint desta classe como PreviewPawnClass em vez do BP do herói para menos custo ao spawn.
 */
UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFClassPreviewCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ADFClassPreviewCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
};
