// Source/DungeonForged/Public/Animation/UDFAnimNotify_SpawnTrailVFX.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "UDFAnimNotify_SpawnTrailVFX.generated.h"

class UNiagaraSystem;
class USkeletalMeshComponent;

/** Which skeletal mesh owns AttachSocket for spawn / re-attach. */
UENUM(BlueprintType)
enum class EDFTrailVFXAttachMesh : uint8
{
	/** Body mesh from the anim notify (e.g. hand_r / hand_l on the character skeleton). */
	CharacterHands UMETA(DisplayName = "Character (Hands)"),
	/** Modular weapon mesh on the owner (Mesh_Weapon), when present. */
	WeaponMesh UMETA(DisplayName = "Weapon Mesh"),
};

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

	/** Body mesh (hands) vs modular weapon mesh (Mesh_Weapon). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
	EDFTrailVFXAttachMesh AttachMesh = EDFTrailVFXAttachMesh::CharacterHands;

	/** Socket on the mesh selected by AttachMesh (e.g. hand_r on body, weapon_tip on sword). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
	FName AttachSocket = FName(TEXT("hand_r"));

	/** Required for reuse / prune / DisableTrailVFX. Leave empty only if every swing may spawn a new system. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
	FName ComponentTag = FName(TEXT("WeaponTrailVFX"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
	bool bSpawnIfMissing = true;
};
