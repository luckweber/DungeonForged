// Source/DungeonForged/Public/Performance/UDFPoolable.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UDFPoolable.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UUDFPoolable : public UInterface
{
	GENERATED_BODY()
};

/** Actors returned by @c UDFObjectPoolSubsystem can reset state on acquire/release. */
class IUDFPoolable
{
	GENERATED_BODY()

public:
	virtual void OnAcquiredFromPool() {}
	virtual void OnReleasedToPool() {}
	virtual FName GetPoolName() const { return NAME_None; }
};
