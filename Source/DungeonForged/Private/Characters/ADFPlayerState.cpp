// Source/DungeonForged/Private/Characters/ADFPlayerState.cpp

#include "Characters/ADFPlayerState.h"
#include "AbilitySystemComponent.h"
#include "GAS/UDFAttributeSet.h"

ADFPlayerState::ADFPlayerState()
{
	// Replication tick rate (was SetNetUpdateFrequency; direct member works with all include orders / IWYU)
	NetUpdateFrequency = 100.f;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UDFAttributeSet>(TEXT("AttributeSet"));

	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

UAbilitySystemComponent* ADFPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ADFPlayerState::BeginPlay()
{
	Super::BeginPlay();
}

void ADFPlayerState::GrantAbilitiesFromDataTable(UDataTable* AbilityTable)
{
	if (!HasAuthority() || !AbilitySystemComponent || !AbilityTable)
	{
		return;
	}

	static const FString Ctx(TEXT("ADFPlayerState::GrantAbilitiesFromDataTable"));
	AbilityTable->ForeachRow<FDFAbilityTableRow>(Ctx, [this](const FName& RowKey, const FDFAbilityTableRow& Row) {
		if (!Row.AbilityClass)
		{
			return;
		}
		const FGameplayAbilitySpec Spec(Row.AbilityClass, Row.AbilityLevel, INDEX_NONE, this);
		AbilitySystemComponent->GiveAbility(Spec);
	});
}

void ADFPlayerState::InitializeAttributesFromDataTable(UDataTable* AttributeTable, FName RowName)
{
	if (!HasAuthority() || !AbilitySystemComponent || !AttributeTable || RowName.IsNone())
	{
		return;
	}

	static const FString Ctx(TEXT("ADFPlayerState::InitializeAttributesFromDataTable"));
	const FDFAttributeInitTableRow* Row = AttributeTable->FindRow<FDFAttributeInitTableRow>(RowName, Ctx, false);
	if (!Row || !Row->StartupGameplayEffect)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle =
		AbilitySystemComponent->MakeOutgoingSpec(Row->StartupGameplayEffect, 1.f, EffectContext);
	if (SpecHandle.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}
