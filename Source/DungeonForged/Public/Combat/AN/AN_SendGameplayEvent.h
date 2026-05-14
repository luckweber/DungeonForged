// Source/DungeonForged/Public/Combat/AN/AN_SendGameplayEvent.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AN_SendGameplayEvent.generated.h"

UCLASS(Blueprintable, meta = (DisplayName = "DF Send Gameplay Event"))
class DUNGEONFORGED_API UAN_SendGameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|GameplayEvent", meta = (Categories = "Event"))
	FGameplayTag EventTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|GameplayEvent")
	float EventMagnitude = 0.f;
};
