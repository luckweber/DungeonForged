// Source/DungeonForged/Public/Audio/UDFMusicManagerSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UDFMusicManagerSubsystem.generated.h"

class ADFMusicLayerHost;
class UAbilitySystemComponent;
class UAudioComponent;
class USoundBase;
struct FGameplayTag;

UENUM(BlueprintType)
enum class EMusicState : uint8
{
	MainMenu	UMETA(DisplayName = "Main Menu"),
	Exploration	UMETA(DisplayName = "Exploration"),
	Combat		UMETA(DisplayName = "Combat"),
	Elite		UMETA(DisplayName = "Elite"),
	Boss		UMETA(DisplayName = "Boss"),
	Victory		UMETA(DisplayName = "Victory"),
	Death		UMETA(DisplayName = "Death"),
};

UCLASS()
class DUNGEONFORGED_API UDFMusicManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Bind State.InCombat on the local player's ASC; call from character after GAS is ready. */
	UFUNCTION(BlueprintCallable, Category = "DF|Audio|Music")
	void RegisterLocalPlayerForCombatMusic(UAbilitySystemComponent* PlayerASC, AActor* OwnerForCleanup);

	UFUNCTION(BlueprintCallable, Category = "DF|Audio|Music")
	void UnregisterLocalPlayerForCombatMusic();

	UFUNCTION(BlueprintCallable, Category = "DF|Audio|Music")
	void SetMusicState(EMusicState NewState);

	/** Boss encounter started (call from boss BP or encounter manager). */
	UFUNCTION(BlueprintCallable, Category = "DF|Audio|Music")
	void OnBossEncounterStarted();

	UFUNCTION(BlueprintCallable, Category = "DF|Audio|Music")
	void OnBossDefeated();

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Audio|Music")
	EMusicState CurrentState = EMusicState::Exploration;

	/** Fades between layer volume targets; default 2s. */
	UPROPERTY(EditAnywhere, Category = "DF|Audio|Music|Mix")
	float CrossfadeDuration = 2.f;

	UPROPERTY(EditAnywhere, Category = "DF|Audio|Music|Assets")
	TObjectPtr<USoundBase> SoundExplorationBase = nullptr;

	UPROPERTY(EditAnywhere, Category = "DF|Audio|Music|Assets")
	TObjectPtr<USoundBase> SoundCombat = nullptr;

	UPROPERTY(EditAnywhere, Category = "DF|Audio|Music|Assets")
	TObjectPtr<USoundBase> SoundBoss = nullptr;

	UPROPERTY(EditAnywhere, Category = "DF|Audio|Music|Assets")
	TObjectPtr<USoundBase> SoundMainMenu = nullptr;

	/** 8s one-shot, then return to exploration. */
	UPROPERTY(EditAnywhere, Category = "DF|Audio|Music|Assets")
	TObjectPtr<USoundBase> StingVictory = nullptr;

	UPROPERTY(EditAnywhere, Category = "DF|Audio|Music|Assets")
	TObjectPtr<USoundBase> StingDeath = nullptr;

protected:
	UFUNCTION()
	void OnInCombatTagChanged(const FGameplayTag Tag, int32 NewCount);

	UFUNCTION()
	void OnCombatClearedToExploration();

	UFUNCTION()
	void OnVictoryReturnToExploration();

	void ApplyInitialExploration();
	void CrossfadeToStateInternal(EMusicState Target);
	void StartVolumeLerp();
	UFUNCTION()
	void TickLayerVolumes();
	void SetLayerTargetVolumes(float Base, float Combat, float Boss);
	void AssignLoopingSoundIfNeeded(
		UAudioComponent* C,
		USoundBase* NewSound,
		bool bShouldBePlaying) const;
	bool ShouldRunMusic() const;
	void PlayDeathSting() const;
	void PlayVictorySting() const;

	TWeakObjectPtr<ADFMusicLayerHost> Host;
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	FDelegateHandle CombatTagHandle;
	FTimerHandle CrossfadeLerpHandle;
	FTimerHandle CombatExploreTimer;
	FTimerHandle VictoryReturnTimer;
	float TBase = 0.f, TCombat = 0.f, TBoss = 0.f;
	float CBase = 0.f, CCombat = 0.f, CBoss = 0.f;
	int32 LerpFrameBudget = 0;
};
