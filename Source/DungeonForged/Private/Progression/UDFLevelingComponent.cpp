// Source/DungeonForged/Private/Progression/UDFLevelingComponent.cpp

#include "Progression/UDFLevelingComponent.h"
#include "Progression/DFLevelingData.h"
#include "Engine/Engine.h"
#include "GAS/DFGameplayTags.h"
#include "UI/Combat/DFCombatTextTypes.h"
#include "UI/Combat/UDFCombatTextSubsystem.h"
#include "GAS/Effects/UGE_LevelUp_StatScaling.h"
#include "GAS/Effects/UGE_Leveling_SpendAttribute.h"
#include "GAS/UDFAttributeSet.h"
#include "Data/DFDataTableStructs.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UDFLevelingComponent)

UDFLevelingComponent::UDFLevelingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UDFLevelingComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* const O = GetOwner();
		O && O->HasAuthority() && LevelTable)
	{
		if (const FDFLevelTableRow* const R = FindRowForLevel(CurrentLevel))
		{
			if (!LevelScalingHandle.IsValid())
			{
				ApplyLevelStatScalingForCurrentRow(*R);
			}
		}
		UpdateCharacterLevelAttribute();
		UpdateLevelGameplayTags();
	}
}

void UDFLevelingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UDFLevelingComponent, CurrentXP);
	DOREPLIFETIME(UDFLevelingComponent, CurrentLevel);
	DOREPLIFETIME(UDFLevelingComponent, UnspentAttributePoints);
}

UAbilitySystemComponent* UDFLevelingComponent::GetOwnerASC() const
{
	if (APlayerState* const PS = Cast<APlayerState>(GetOwner()))
	{
		if (IAbilitySystemInterface* const I = Cast<IAbilitySystemInterface>(PS))
		{
			return I->GetAbilitySystemComponent();
		}
	}
	return nullptr;
}

UDFAttributeSet* UDFLevelingComponent::GetOwnerAttributeSet() const
{
	if (UAbilitySystemComponent* const ASC = GetOwnerASC())
	{
		return const_cast<UDFAttributeSet*>(ASC->GetSet<UDFAttributeSet>());
	}
	return nullptr;
}

const FDFLevelTableRow* UDFLevelingComponent::FindRowForLevel(const int32 Level) const
{
	if (!LevelTable || Level < 1)
	{
		return nullptr;
	}
	const FDFLevelTableRow* Found = nullptr;
	LevelTable->ForeachRow<FDFLevelTableRow>(TEXT("UDFLevelingComponent::FindRowForLevel"), [&Level, &Found](
												   const FName& /*Key*/, const FDFLevelTableRow& R)
	{
		if (R.Level == Level)
		{
			Found = &R;
		}
	});
	return Found;
}

const FDFLevelTableRow* UDFLevelingComponent::FindRowForNextLevel() const
{
	return FindRowForLevel(CurrentLevel + 1);
}

int32 UDFLevelingComponent::GetXPToNextLevel() const
{
	if (CurrentLevel >= MaxLevel)
	{
		return 0;
	}
	if (const FDFLevelTableRow* const NextR = FindRowForNextLevel())
	{
		return FMath::Max(0, NextR->XPRequired - CurrentXP);
	}
	return 0;
}

float UDFLevelingComponent::GetXPProgress() const
{
	if (CurrentLevel >= MaxLevel)
	{
		return 1.f;
	}
	if (const FDFLevelTableRow* const NextR = FindRowForNextLevel())
	{
		const int32 XPEnd = NextR->XPRequired;
		if (const FDFLevelTableRow* const CurR = FindRowForLevel(CurrentLevel))
		{
			const int32 XPStart = CurR->XPRequired;
			const int32 Den = FMath::Max(1, XPEnd - XPStart);
			return FMath::Clamp(
				static_cast<float>(CurrentXP - XPStart) / static_cast<float>(Den), 0.f, 1.f);
		}
		// no row for current: fall back
		if (XPEnd > 0)
		{
			return FMath::Clamp(
				static_cast<float>(CurrentXP) / static_cast<float>(FMath::Max(1, XPEnd)), 0.f, 1.f);
		}
	}
	return 0.f;
}

void UDFLevelingComponent::BroadcastXP()
{
	OnXPChanged.Broadcast(CurrentXP, GetXPToNextLevel());
}

void UDFLevelingComponent::OnRep_CurrentXP()
{
	BroadcastXP();
}

void UDFLevelingComponent::OnRep_UnspentAttributePoints()
{
}

void UDFLevelingComponent::OnRep_CurrentLevel()
{
	if (UWorld* const W = GetWorld();
		W && W->GetNetMode() == NM_Client)
	{
		if (ClientLastReplicatedLevel > 0 && CurrentLevel > ClientLastReplicatedLevel)
		{
			if (!IsRunningDedicatedServer())
			{
				PlayLevelUpCosmetics();
			}
			OnLevelUp.Broadcast(CurrentLevel, UnspentAttributePoints);
		}
		ClientLastReplicatedLevel = CurrentLevel;
	}
	BroadcastXP();
}

void UDFLevelingComponent::AddXP(const int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	if (APlayerState* const PS = Cast<APlayerState>(GetOwner()))
	{
		if (!PS->HasAuthority())
		{
			return;
		}
	}
	else
	{
		return;
	}
	CurrentXP += Amount;
	CheckLevelUp();
	BroadcastXP();
	if (!IsRunningDedicatedServer())
	{
		if (APlayerState* const PS = Cast<APlayerState>(GetOwner()))
		{
			const FVector L = (PS->GetPawn() ? PS->GetPawn() : static_cast<AActor*>(PS))
				->GetActorLocation()
				+ FVector(0.f, 0.f, 100.f);
			if (UWorld* const W = GetWorld())
			{
				if (UDFCombatTextSubsystem* const Ctx = W->GetSubsystem<UDFCombatTextSubsystem>())
				{
					Ctx->SpawnText(L, static_cast<float>(Amount), ECombatTextType::XPGain);
				}
			}
		}
	}
}

void UDFLevelingComponent::CheckLevelUp()
{
	while (CurrentLevel < MaxLevel)
	{
		if (const FDFLevelTableRow* const NextR = FindRowForNextLevel())
		{
			if (CurrentXP < NextR->XPRequired)
			{
				break;
			}
			LevelUp();
		}
		else
		{
			break;
		}
	}
}

void UDFLevelingComponent::UpdateCharacterLevelAttribute() const
{
	if (UAbilitySystemComponent* const ASC = GetOwnerASC())
	{
		if (UDFAttributeSet* const S = const_cast<UDFAttributeSet*>(ASC->GetSet<UDFAttributeSet>()))
		{
			S->SetCharacterLevel(static_cast<float>(CurrentLevel));
		}
	}
}

void UDFLevelingComponent::UpdateLevelGameplayTags()
{
	UAbilitySystemComponent* const ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}
	if (RepTagLevel > 0)
	{
		if (FGameplayTag OldT = FGameplayTag::RequestGameplayTag(
				FName(*FString::Printf(TEXT("Character.Level.%d"), RepTagLevel)), false);
			OldT.IsValid())
		{
			ASC->RemoveLooseGameplayTag(OldT, 1);
		}
	}
	RepTagLevel = CurrentLevel;
	if (FGameplayTag NewT = FGameplayTag::RequestGameplayTag(
			FName(*FString::Printf(TEXT("Character.Level.%d"), CurrentLevel)), false);
		NewT.IsValid())
	{
		ASC->AddLooseGameplayTag(NewT, 1);
	}
}

void UDFLevelingComponent::ApplyLevelStatScalingForCurrentRow(const FDFLevelTableRow& Row)
{
	UAbilitySystemComponent* const ASC = GetOwnerASC();
	if (!ASC || !ASC->GetAvatarActor())
	{
		return;
	}
	if (LevelScalingHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(LevelScalingHandle, 1);
		LevelScalingHandle = FActiveGameplayEffectHandle();
	}
	const float BaseHealth = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
	const float BaseMana = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxManaAttribute());
	const float M = FMath::Max(0.01f, Row.StatScalingMultiplier);
	const float AddH = FMath::Max(0.f, BaseHealth * (M - 1.f));
	const float AddM = FMath::Max(0.f, BaseMana * (M - 1.f));
	const float L = static_cast<float>(CurrentLevel);
	const float StrA = L * 2.f;
	const float IntA = L * 2.f;
	const float AgiA = L * 1.f;

	FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
	if (APlayerState* const PS = Cast<APlayerState>(GetOwner()))
	{
		Ctx.AddSourceObject(PS);
	}
	const FGameplayEffectSpecHandle H = ASC->MakeOutgoingSpec(UGE_LevelUp_StatScaling::StaticClass(), 1.f, Ctx);
	if (H.IsValid() && H.Data)
	{
		if (FDFGameplayTags::Data_LevelUp_MaxHealthAdd.IsValid())
		{
			H.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_LevelUp_MaxHealthAdd, AddH);
		}
		if (FDFGameplayTags::Data_LevelUp_MaxManaAdd.IsValid())
		{
			H.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_LevelUp_MaxManaAdd, AddM);
		}
		if (FDFGameplayTags::Data_LevelUp_StrengthAdd.IsValid())
		{
			H.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_LevelUp_StrengthAdd, StrA);
		}
		if (FDFGameplayTags::Data_LevelUp_IntelligenceAdd.IsValid())
		{
			H.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_LevelUp_IntelligenceAdd, IntA);
		}
		if (FDFGameplayTags::Data_LevelUp_AgilityAdd.IsValid())
		{
			H.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_LevelUp_AgilityAdd, AgiA);
		}
		LevelScalingHandle = ASC->ApplyGameplayEffectSpecToSelf(*H.Data);
	}
}

void UDFLevelingComponent::GrantUnlockedAbilitiesForRow(const FDFLevelTableRow& Row) const
{
	if (Row.AbilitiesUnlocked.IsEmpty() || !AbilityUnlockTable)
	{
		return;
	}
	if (APlayerState* const PS = Cast<APlayerState>(GetOwner()))
	{
		if (UAbilitySystemComponent* const ASC = GetOwnerASC())
		{
			for (FName R : Row.AbilitiesUnlocked)
			{
				if (R.IsNone())
				{
					continue;
				}
				static const FString CtxName(TEXT("UDFLevelingComponent::GrantUnlocked"));
				if (const FDFAbilityTableRow* const Ab =
						AbilityUnlockTable->FindRow<FDFAbilityTableRow>(R, CtxName, false);
					Ab && Ab->AbilityClass)
				{
					const FGameplayAbilitySpec S(Ab->AbilityClass, Ab->AbilityLevel, INDEX_NONE, PS);
					ASC->GiveAbility(S);
				}
			}
		}
	}
}

void UDFLevelingComponent::PlayLevelUpCosmetics() const
{
	if (IsRunningDedicatedServer() || !GetWorld())
	{
		return;
	}
	if (APlayerState* const PS = Cast<APlayerState>(GetOwner()))
	{
		APawn* Pawn = PS->GetPawn();
		if (Pawn)
		{
			if (AController* C = Pawn->GetController())
			{
				if (C->IsLocalController() && LevelUpFanfare)
				{
					UGameplayStatics::PlaySound2D(
						this, LevelUpFanfare, 1.f, 0.f, 0.15f, nullptr, nullptr, false);
				}
			}
		}
		if (LevelUpNiagara)
		{
			const FVector L = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(), LevelUpNiagara, L, FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::None, true);
		}
	}
}

void UDFLevelingComponent::LevelUp()
{
	CurrentLevel = FMath::Min(CurrentLevel + 1, MaxLevel);
	if (const FDFLevelTableRow* R = FindRowForLevel(CurrentLevel))
	{
		UnspentAttributePoints += FMath::Max(0, R->AttributePointsGranted);
		ApplyLevelStatScalingForCurrentRow(*R);
		GrantUnlockedAbilitiesForRow(*R);
	}
	UpdateCharacterLevelAttribute();
	UpdateLevelGameplayTags();
	if (UWorld* W = GetWorld();
		W && W->GetNetMode() != NM_DedicatedServer)
	{
		BroadcastXP();
		OnLevelUp.Broadcast(CurrentLevel, UnspentAttributePoints);
		PlayLevelUpCosmetics();
		if (!IsRunningDedicatedServer())
		{
			if (APlayerState* const PS = Cast<APlayerState>(GetOwner()))
			{
				const AActor* const Anchor = PS->GetPawn() ? static_cast<AActor*>(PS->GetPawn()) : static_cast<AActor*>(PS);
				const FVector L = (Anchor ? Anchor->GetActorLocation() : FVector::ZeroVector) + FVector(0.f, 0.f, 120.f);
				if (UDFCombatTextSubsystem* const Cts = W->GetSubsystem<UDFCombatTextSubsystem>())
				{
					Cts->SpawnTextString(L, TEXT("LEVEL UP!"), ECombatTextType::LevelUp, 2.f);
				}
			}
		}
	}
}

void UDFLevelingComponent::SpendAttributePoint(EDFLevelingStat const Stat, int32 const Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	AActor* const O = GetOwner();
	if (O && !O->HasAuthority())
	{
		Server_SpendAttributePoint(Stat, Amount);
		return;
	}
	if (!O || !O->HasAuthority())
	{
		return;
	}
	if (UnspentAttributePoints < Amount)
	{
		return;
	}
	if (UAbilitySystemComponent* const ASC = GetOwnerASC();
		ASC && ASC->GetAvatarActor())
	{
		TSubclassOf<UGameplayEffect> Ge = nullptr;
		switch (Stat)
		{
		case EDFLevelingStat::Strength: Ge = UGE_Leveling_Spend_Strength::StaticClass();
			break;
		case EDFLevelingStat::Intelligence: Ge = UGE_Leveling_Spend_Intelligence::StaticClass();
			break;
		case EDFLevelingStat::Agility: Ge = UGE_Leveling_Spend_Agility::StaticClass();
			break;
		}
		if (!Ge)
		{
			return;
		}
		FGameplayEffectContextHandle Cx = ASC->MakeEffectContext();
		Cx.AddSourceObject(GetOwner());
		const FGameplayEffectSpecHandle Sh = ASC->MakeOutgoingSpec(Ge, 1.f, Cx);
		if (Sh.IsValid() && Sh.Data)
		{
			if (FDFGameplayTags::Data_Magnitude.IsValid())
			{
				Sh.Data->SetSetByCallerMagnitude(
					FDFGameplayTags::Data_Magnitude, static_cast<float>(Amount));
			}
			ASC->ApplyGameplayEffectSpecToSelf(*Sh.Data);
		}
		UnspentAttributePoints -= Amount;
		OnAttributePointSpent.Broadcast(Stat, Amount, UnspentAttributePoints);
	}
}

bool UDFLevelingComponent::Server_SpendAttributePoint_Validate(
	EDFLevelingStat const /*Stat*/, int32 const Amount)
{
	return Amount > 0 && Amount <= 100;
}

void UDFLevelingComponent::Server_SpendAttributePoint_Implementation(
	EDFLevelingStat const Stat, int32 const Amount)
{
	SpendAttributePoint(Stat, Amount);
}
