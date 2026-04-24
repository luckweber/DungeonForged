// Source/DungeonForged/Public/Progression/UDFLevelingComponent.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Progression/DFLevelingData.h"
#include "Components/ActorComponent.h"
#include "AttributeSet.h"
#include "UDFLevelingComponent.generated.h"

class UDataTable;
class UAbilitySystemComponent;
class UDFAttributeSet;
class UGameplayEffect;
class UNiagaraSystem;
class USoundBase;

UENUM(BlueprintType)
enum class EDFLevelingStat : uint8
{
	Strength,
	Intelligence,
	Agility
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDFXPChanged, int32 /*CurrentXP*/, int32 /*XPToNext*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDFLevelUp, int32 /*NewLevel*/, int32 /*UnspentAttributePoints*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnDFAttributePointSpent, EDFLevelingStat /*Stat*/, int32 /*SpentAmount*/, int32 /*RemainingUnspent*/);

UCLASS(Blueprintable, ClassGroup = (DF), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFLevelingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFLevelingComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Cumulative; gates level thresholds from LevelTable. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentXP, Category = "DF|Leveling")
	int32 CurrentXP = 0;

	/** 1 = starting level. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentLevel, Category = "DF|Leveling")
	int32 CurrentLevel = 1;

	/** Unspent from level rewards; spent on primary stats. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_UnspentAttributePoints, Category = "DF|Leveling")
	int32 UnspentAttributePoints = 0;

	/** FDFLevelTableRow table (DT_Levels). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Leveling")
	TObjectPtr<UDataTable> LevelTable = nullptr;

	/** DT_Abilities (FDFAbilityTableRow) for AbilitiesUnlocked row names. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Leveling")
	TObjectPtr<UDataTable> AbilityUnlockTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Leveling", meta = (ClampMin = "1"))
	int32 MaxLevel = 30;

	/** Fired on XP amount change (and after level-up resync). */
	FOnDFXPChanged OnXPChanged;

	/**
	 * Fired when level increases: on server (non-dedicated) in LevelUp, and on clients in OnRep_CurrentLevel
	 * with NM_Client. Bind UI / WBP here.
	 */
	FOnDFLevelUp OnLevelUp;

	/** Fired on successful spend. */
	FOnDFAttributePointSpent OnAttributePointSpent;

	/** Authority: add XP, roll level-ups, grant rewards. */
	UFUNCTION(BlueprintCallable, Category = "DF|Leveling")
	void AddXP(int32 Amount);

	/** Client & server: request spend. Routes to server if needed. */
	UFUNCTION(BlueprintCallable, Category = "DF|Leveling")
	void SpendAttributePoint(EDFLevelingStat Stat, int32 Amount = 1);

	/** XP from CurrentXP to the next level threshold, or 0 if max. */
	UFUNCTION(BlueprintPure, Category = "DF|Leveling")
	int32 GetXPToNextLevel() const;

	/** 0..1 fill between CurrentLevel and next threshold. 1.0 if max. */
	UFUNCTION(BlueprintPure, Category = "DF|Leveling")
	float GetXPProgress() const;

	UFUNCTION(BlueprintPure, Category = "DF|Leveling")
	UAbilitySystemComponent* GetOwnerASC() const;

	UFUNCTION(BlueprintPure, Category = "DF|Leveling")
	UDFAttributeSet* GetOwnerAttributeSet() const;

	UPROPERTY(EditAnywhere, Category = "DF|Leveling|VFX", meta = (DisplayName = "Level Up VFX (optional)"))
	TObjectPtr<UNiagaraSystem> LevelUpNiagara = nullptr;

	UPROPERTY(EditAnywhere, Category = "DF|Leveling|VFX", meta = (DisplayName = "Level Up SFX (optional)"))
	TObjectPtr<USoundBase> LevelUpFanfare = nullptr;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_CurrentXP();
	UFUNCTION()
	void OnRep_CurrentLevel();
	UFUNCTION()
	void OnRep_UnspentAttributePoints();

	void CheckLevelUp();
	void LevelUp();
	void ApplyLevelStatScalingForCurrentRow(const FDFLevelTableRow& Row);
	void GrantUnlockedAbilitiesForRow(const FDFLevelTableRow& Row) const;
	void UpdateCharacterLevelAttribute() const;
	void UpdateLevelGameplayTags();
	void PlayLevelUpCosmetics() const;
	void BroadcastXP();

	const FDFLevelTableRow* FindRowForLevel(int32 Level) const;
	const FDFLevelTableRow* FindRowForNextLevel() const; // current + 1

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SpendAttributePoint(EDFLevelingStat Stat, int32 Amount);

	/** Replaced each level. */
	FActiveGameplayEffectHandle LevelScalingHandle;

	/** For tag cleanup on level; 0 = no prior tag to remove. */
	int32 RepTagLevel = 0;

	/** Client: avoids firing OnLevelUp on initial replication. */
	int32 ClientLastReplicatedLevel = 0;
};
