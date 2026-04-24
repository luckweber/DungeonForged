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
	FString DefaultHostTravelURL = TEXT("/Game/Maps/Entry?listen");

	/** `OpenLevel` when leaving. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Session")
	FString MainMenuMapName = TEXT("/Game/Maps/Entry");

	UFUNCTION(BlueprintCallable, Category = "DF|Session")
	void HostSession();

	UFUNCTION(BlueprintCallable, Category = "DF|Session")
	void JoinSession(const FString& Address);

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
