// Source/DungeonForged/Public/Animation/UDFAnimNotify_SpawnTrailVFX.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "UDFAnimNotify_SpawnTrailVFX.generated.h"

class UNiagaraSystem;

/**
 * Activates a tagged Niagara component on the mesh owner, or (if bSpawnIfMissing) spawns
 * and attaches Template at AttachSocket, optionally tagging for DisableTrailVFX.
 */
UCLASS(Blueprintable, const, meta = (DisplayName = "DF | Enable Weapon Trail VFX"))
class DUNGEONFORGED_API UUDFAnimNotify_SpawnTrailVFX : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
	TObjectPtr<UNiagaraSystem> Template;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
	FName AttachSocket = FName(TEXT("hand_r"));

	/** Tries to find and activate a Niagara on the owner (or on mesh) with this tag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
	FName ComponentTag = FName(TEXT("WeaponTrailVFX"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
	bool bSpawnIfMissing = true;
};
