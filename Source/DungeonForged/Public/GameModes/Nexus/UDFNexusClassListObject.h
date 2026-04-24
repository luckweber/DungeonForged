// Source/DungeonForged/Public/GameModes/Nexus/UDFNexusClassListObject.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UDFNexusClassListObject.generated.h"

UCLASS(Blueprintable)
class DUNGEONFORGED_API UDFNexusClassListObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Nexus|UI")
	FName ClassRow = NAME_None;

	UPROPERTY(BlueprintReadWrite, Category = "Nexus|UI")
	FText Name;

	UPROPERTY(BlueprintReadWrite, Category = "Nexus|UI")
	FText Blurb;

	UPROPERTY(BlueprintReadWrite, Category = "Nexus|UI")
	bool bLocked = false;

	UPROPERTY(BlueprintReadWrite, Category = "Nexus|UI")
	FText LockHint;
};
