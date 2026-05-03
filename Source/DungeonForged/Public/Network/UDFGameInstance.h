// Source/DungeonForged/Public/Network/UDFGameInstance.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "UDFGameInstance.generated.h"

/**
 * Process-wide GameInstance: GAS + NativeGameplayTags init, optional LAN/OnlineSubsystem session
 * helpers for up to `MaxPlayers` (co-op: 2).
 * Config: `DefaultEngine.ini` -> `GameInstanceClass=/Script/DungeonForged.UDFGameInstance`
 */
UCLASS()
class DUNGEONFORGED_API UDFGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Session")
	bool bIsOnlineSession = false;

	/** Last join / connect string (e.g. `127.0.0.1:17777`). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Session")
	FString ServerAddress;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Session", meta = (ClampMin = "1", UIMin = 1))
	int32 MaxPlayers = 2;

	/** `ServerTravel` when hosting without null OSS, or if a session already exists. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Session")
	FString DefaultHostTravelURL = TEXT("/Game/DungeonForged/Maps/L_Nexus?listen");

	/** `OpenLevel` when leaving (i.e. returning to the main menu). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Session")
	FString MainMenuMapName = TEXT("/Game/DungeonForged/Maps/L_MainMenu");

	UFUNCTION(BlueprintCallable, Category = "DF|Session")
	void HostSession();

	/**
	 * Direct ClientTravel to Address (e.g. 127.0.0.1:17777).
	 * This is not Online Subsystem UGameInstance::JoinSession; named separately so the base virtual is not hidden.
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|Session", meta = (DisplayName = "Join Session (By Address)"))
	void JoinSessionToAddress(const FString& Address);

	UFUNCTION(BlueprintCallable, Category = "DF|Session")
	void LeaveSession();

protected:
	void OnCreateSessionComplete(FName const SessionName, bool const bWasSuccessful);
	void OnDestroySessionComplete(FName const SessionName, bool const bWasSuccessful);
	void OnDestroyBeforeHost(FName const SessionName, bool const bWasSuccessful);
	void DoCreateSessionInternal();
	void StartListenTravel();

private:
	bool bCreateSessionInFlight = false;
	bool bPendingCreateAfterHostDestroy = false;
	FDelegateHandle PendingCreateSessionHandle;
};
