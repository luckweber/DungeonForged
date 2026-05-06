// Source/DungeonForgedEditor/Public/EditorIconBake/UDFEditorIconStudioSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "UDFEditorIconStudioSubsystem.generated.h"

/** Opcionalmente abre o EUW de icon bake ao abrir o mapa configurado em UDFIconStudioWorkflowSettings. */
UCLASS()
class DUNGEONFORGEDEDITOR_API UDFEditorIconStudioSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void HandleMapOpened(const FString& Filename, bool bLoadAsTemplate);
};
