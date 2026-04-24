// Source/DungeonForged/Public/Performance/UDFPerformanceSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/WeakObjectPtr.h"

#include "UDFPerformanceSubsystem.generated.h"

class UDFObjectPoolSubsystem;
class UDFRoomCullingComponent;

UCLASS()
class DUNGEONFORGED_API UDFPerformanceSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/** Registry of **names** of pools that exist on `UDFObjectPoolSubsystem` (combat text uses `UDFCombatTextSubsystem` separately). */
	UPROPERTY(VisibleAnywhere, Category = "DF|Performance")
	TMap<FName, bool> PoolRegistry;

	UPROPERTY(EditAnywhere, Category = "DF|Performance")
	float ProfilingInterval = 5.f;

	float LastProfilingTime = 0.f;

	UFUNCTION(BlueprintCallable, Category = "DF|Performance")
	void RegisterPoolEntry(FName PoolName, bool bRegistered = true);

	UFUNCTION(BlueprintCallable, Category = "DF|Performance")
	UDFObjectPoolSubsystem* GetObjectPoolSubsystem() const;

	UFUNCTION(BlueprintCallable, Category = "DF|Performance")
	void TickProfiling(float DeltaTime);

	void RegisterRoomCulling(UDFRoomCullingComponent* Culling);
	void UnregisterRoomCulling(UDFRoomCullingComponent* Culling);

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
	virtual bool IsTickableWhenPaused() const override { return false; }

protected:
	/** 1s cadence for room culling vs player position. */
	float RoomCullAccum = 0.f;
	static constexpr float RoomCullInterval = 1.f;

	TArray<TWeakObjectPtr<UDFRoomCullingComponent>> RoomCullers;

	void UpdateRoomCulling();
};
