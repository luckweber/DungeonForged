// Source/DungeonForged/Private/Network/UDFGameInstance.cpp

#include "Network/UDFGameInstance.h"
#include "AbilitySystemGlobals.h"
#include "GAS/DFGameplayTags.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"

void UDFGameInstance::Init()
{
	Super::Init();
	FDFGameplayTags::RegisterGameplayTags();
	UAbilitySystemGlobals::Get().InitGlobalData();
}

void UDFGameInstance::StartListenTravel()
{
	if (UWorld* const W = GetWorld())
	{
		if (W->GetNetMode() == NM_Client)
		{
			return;
		}
		if (!DefaultHostTravelURL.IsEmpty())
		{
			W->ServerTravel(DefaultHostTravelURL, false);
		}
	}
}

void UDFGameInstance::OnCreateSessionComplete(const FName SessionName, const bool bWasSuccessful)
{
	if (IOnlineSubsystem* const OS = IOnlineSubsystem::Get())
	{
		if (IOnlineSessionPtr const S = OS->GetSessionInterface())
		{
			if (PendingCreateSessionHandle.IsValid())
			{
				S->ClearOnCreateSessionCompleteDelegate_Handle(PendingCreateSessionHandle);
				PendingCreateSessionHandle.Reset();
			}
		}
	}
	bCreateSessionInFlight = false;
	if (bWasSuccessful)
	{
		bIsOnlineSession = true;
		StartListenTravel();
	}
}

void UDFGameInstance::OnDestroySessionComplete(const FName SessionName, const bool bWasSuccessful)
{
	bIsOnlineSession = false;
	UGameplayStatics::OpenLevel(this, FName(*MainMenuMapName));
}

void UDFGameInstance::OnDestroyBeforeHost(const FName SessionName, const bool bWasSuccessful)
{
	bPendingCreateAfterHostDestroy = false;
	DoCreateSessionInternal();
}

void UDFGameInstance::DoCreateSessionInternal()
{
	IOnlineSubsystem* const OS = IOnlineSubsystem::Get();
	if (!OS)
	{
		StartListenTravel();
		return;
	}
	const IOnlineSessionPtr S = OS->GetSessionInterface();
	if (!S.IsValid())
	{
		StartListenTravel();
		return;
	}
	if (S->GetNamedSession(NAME_GameSession))
	{
		if (!bPendingCreateAfterHostDestroy)
		{
			bIsOnlineSession = true;
			StartListenTravel();
		}
		return;
	}
	bCreateSessionInFlight = true;
	const TSharedRef<FOnlineSessionSettings> Settings = MakeShared<FOnlineSessionSettings>();
	Settings->bIsLANMatch = true;
	Settings->NumPublicConnections = FMath::Max(1, MaxPlayers);
	Settings->bShouldAdvertise = true;
	Settings->bAllowJoinInProgress = true;
	Settings->bIsDedicated = false;
	Settings->bUseLobbiesIfAvailable = true;
	Settings->bUsesPresence = true;
	PendingCreateSessionHandle = S->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UDFGameInstance::OnCreateSessionComplete));
	if (!S->CreateSession(0, NAME_GameSession, *Settings))
	{
		S->ClearOnCreateSessionCompleteDelegate_Handle(PendingCreateSessionHandle);
		PendingCreateSessionHandle.Reset();
		bCreateSessionInFlight = false;
		StartListenTravel();
	}
}

void UDFGameInstance::HostSession()
{
	if (UWorld* const W = GetWorld())
	{
		if (W->GetNetMode() == NM_Client)
		{
			return;
		}
	}
	if (!IOnlineSubsystem::Get())
	{
		StartListenTravel();
		return;
	}
	if (IOnlineSessionPtr const S = IOnlineSubsystem::Get()->GetSessionInterface())
	{
		if (S->GetNamedSession(NAME_GameSession) && !bCreateSessionInFlight)
		{
			bPendingCreateAfterHostDestroy = true;
			S->DestroySession(NAME_GameSession, FOnDestroySessionCompleteDelegate::CreateUObject(
				this, &UDFGameInstance::OnDestroyBeforeHost));
			return;
		}
	}
	DoCreateSessionInternal();
}

void UDFGameInstance::JoinSession(const FString& Address)
{
	ServerAddress = Address;
	if (APlayerController* const PC = GetFirstLocalPlayerController())
	{
		PC->ClientTravel(ServerAddress, ETravelType::TRAVEL_Absolute);
	}
}

void UDFGameInstance::LeaveSession()
{
	IOnlineSubsystem* const OS = IOnlineSubsystem::Get();
	const IOnlineSessionPtr S = OS ? OS->GetSessionInterface() : nullptr;
	if (S.IsValid() && S->GetNamedSession(NAME_GameSession))
	{
		if (!S->DestroySession(NAME_GameSession, FOnDestroySessionCompleteDelegate::CreateUObject(
			this, &UDFGameInstance::OnDestroySessionComplete)))
		{
			UGameplayStatics::OpenLevel(this, FName(*MainMenuMapName));
		}
		return;
	}
	UGameplayStatics::OpenLevel(this, FName(*MainMenuMapName));
}
