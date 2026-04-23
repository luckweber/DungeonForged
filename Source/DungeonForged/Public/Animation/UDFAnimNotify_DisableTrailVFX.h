// Source/DungeonForged/Public/Animation/UDFAnimNotify_DisableTrailVFX.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "UDFAnimNotify_DisableTrailVFX.generated.h"

/** Deactivates tagged Niagara systems spawned or tagged by UDFAnimNotify_SpawnTrailVFX. */
UCLASS(Blueprintable, const, meta = (DisplayName = "DF | Disable Weapon Trail VFX"))
class DUNGEONFORGED_API UUDFAnimNotify_DisableTrailVFX : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
	FName ComponentTag = FName(TEXT("WeaponTrailVFX"));
};
