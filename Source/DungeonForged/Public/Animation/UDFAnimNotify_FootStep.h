// Source/DungeonForged/Public/Animation/UDFAnimNotify_FootStep.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "UDFAnimNotify_FootStep.generated.h"

class USoundBase;

/** Traces to ground and plays a USoundBase based on physical material surface (Stone/Dirt/Wood/Metal or default). */
UCLASS(Blueprintable, const, meta = (DisplayName = "DF | Foot Step (Surface Sound)"))
class DUNGEONFORGED_API UUDFAnimNotify_FootStep : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundBase> DefaultSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundBase> Sound_Stone = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundBase> Sound_Dirt = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundBase> Sound_Wood = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundBase> Sound_Metal = nullptr;

	/** If set, this surface from your Physical Materials is treated as "Stone" for sound selection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Surface Map")
	TEnumAsByte<EPhysicalSurface> MapStoneTo = EPhysicalSurface::SurfaceType1;

	/** If set, treated as "Dirt" (grass/mud) — set per project; defaults can overlap until assets exist. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Surface Map")
	TEnumAsByte<EPhysicalSurface> MapDirtTo = EPhysicalSurface::SurfaceType2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Surface Map")
	TEnumAsByte<EPhysicalSurface> MapWoodTo = EPhysicalSurface::SurfaceType3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Surface Map")
	TEnumAsByte<EPhysicalSurface> MapMetalTo = EPhysicalSurface::SurfaceType4;

	/** Skeletal socket for trace origin (e.g. foot). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	FName FootSocketName = FName(TEXT("foot_l"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	float TraceUp = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	float TraceDown = 100.f;
};
