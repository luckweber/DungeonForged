// Source/DungeonForged/Public/Animation/UDFAnimNotify_FootStep.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "UDFAnimNotify_FootStep.generated.h"

class UDFAudioComponent;
class UNiagaraSystem;
class USoundBase;

/**
 * Traces to ground; prefers UDFAudioComponent::PlayFootstep (per-surface DataAsset / maps on the character).
 * If no DFAudio on the owner, falls back to the legacy inline USound* fields and surface mapping.
 */
UCLASS(Blueprintable, const, meta = (DisplayName = "DF | Foot Step (Surface + DFAudio)"))
class DUNGEONFORGED_API UUDFAnimNotify_FootStep : public UAnimNotify
{
	GENERATED_BODY()
public:
	UUDFAnimNotify_FootStep();

	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	/** If set, this surface from your Physical Materials is treated as "Stone" for sound selection (legacy path only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Surface Map (Legacy)")
	TEnumAsByte<EPhysicalSurface> MapStoneTo = EPhysicalSurface::SurfaceType1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Surface Map (Legacy)")
	TEnumAsByte<EPhysicalSurface> MapDirtTo = EPhysicalSurface::SurfaceType2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Surface Map (Legacy)")
	TEnumAsByte<EPhysicalSurface> MapWoodTo = EPhysicalSurface::SurfaceType3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Surface Map (Legacy)")
	TEnumAsByte<EPhysicalSurface> MapMetalTo = EPhysicalSurface::SurfaceType4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Legacy (no DFAudio on owner)")
	TObjectPtr<USoundBase> DefaultSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Legacy (no DFAudio on owner)")
	TObjectPtr<USoundBase> Sound_Stone = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Legacy (no DFAudio on owner)")
	TObjectPtr<USoundBase> Sound_Dirt = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Legacy (no DFAudio on owner)")
	TObjectPtr<USoundBase> Sound_Wood = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Legacy (no DFAudio on owner)")
	TObjectPtr<USoundBase> Sound_Metal = nullptr;

	/** Optional surface -> Niagara (e.g. dust on dirt, spark on metal). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Optional")
	TMap<TEnumAsByte<EPhysicalSurface>, TObjectPtr<UNiagaraSystem>> FootstepVfxBySurface;

	/** Skeletal socket for trace origin (e.g. foot). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	FName FootSocketName = FName(TEXT("foot_l"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace", meta = (ClampMin = "0.0"))
	float TraceUp = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace", meta = (ClampMin = "0.0"))
	float TraceDown = 100.f;
};
