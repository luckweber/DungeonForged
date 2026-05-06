// Copyright DungeonForged.

#include "EditorIconBake/UDFEditorIconBakeWidget.h"

#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AActor* UDFEditorIconBakeWidget::ResolveIconCreatorActor()
{
	if (CachedIconCreatorActor && IsValid(CachedIconCreatorActor))
	{
		return CachedIconCreatorActor;
	}

	if (!IconCreatorActorClass)
	{
		return nullptr;
	}

	UWorld* World = nullptr;
	if (GEditor)
	{
		World = GEditor->GetEditorWorldContext().World();
	}

	if (!World || !World->IsEditorWorld())
	{
		return nullptr;
	}

	for (TActorIterator<AActor> It(World, IconCreatorActorClass); It; ++It)
	{
		AActor* const Found = *It;
		if (Found && IsValid(Found))
		{
			CachedIconCreatorActor = Found;
			return CachedIconCreatorActor;
		}
	}

	CachedIconCreatorActor = nullptr;
	return nullptr;
}

void UDFEditorIconBakeWidget::ClearIconCreatorActorCache()
{
	CachedIconCreatorActor = nullptr;
}
