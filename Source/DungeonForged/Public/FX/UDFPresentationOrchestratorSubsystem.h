// Source/DungeonForged/Public/FX/UDFPresentationOrchestratorSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "FX/UDFCombatFeedbackTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "UDFPresentationOrchestratorSubsystem.generated.h"

class ADFPlayerCharacter;

USTRUCT(BlueprintType)
struct FDFPresentationMoment
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "DF|Presentation")
	EDFHitFeedbackBand HitBand = EDFHitFeedbackBand::Light;

	UPROPERTY(BlueprintReadWrite, Category = "DF|Presentation")
	float DamagePercent = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "DF|Presentation")
	bool bWasCritical = false;

	UPROPERTY(BlueprintReadWrite, Category = "DF|Presentation")
	bool bWasLethal = false;

	UPROPERTY(BlueprintReadWrite, Category = "DF|Presentation")
	TObjectPtr<AActor> Instigator = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "DF|Presentation")
	TObjectPtr<AActor> Victim = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "DF|Presentation")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "DF|Presentation")
	int32 MomentId = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDFPresentationMoment, const FDFPresentationMoment&, Moment);

/**
 * Single dispatch point for coordinated hit-stop / shake / screen FX / combat text beats.
 */
UCLASS()
class DUNGEONFORGED_API UDFPresentationOrchestratorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "DF|Presentation")
	FOnDFPresentationMoment OnPresentationMoment;

	UFUNCTION(BlueprintCallable, Category = "DF|Presentation")
	int32 DispatchFromHitContext(const FDFHitConfirmedContext& Context);

	UFUNCTION(BlueprintPure, Category = "DF|Presentation")
	int32 GetLastMomentId() const { return LastMomentId; }

protected:
	int32 LastMomentId = 0;
};
