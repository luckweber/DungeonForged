// Source/DungeonForged/Private/Performance/UDFNiagaraPoolComponent.cpp
#include "Performance/UDFNiagaraPoolComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "GameFramework/Actor.h"

UDFNiagaraPoolComponent::UDFNiagaraPoolComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDFNiagaraPoolComponent::OnRegister()
{
	Super::OnRegister();
	AActor* const O = GetOwner();
	if (!O || PooledNiagara)
	{
		return;
	}
	PooledNiagara = NewObject<UNiagaraComponent>(O, TEXT("PooledNiagaraVFX"), RF_Transient);
	if (!PooledNiagara)
	{
		return;
	}
	if (PooledSystem)
	{
		PooledNiagara->SetAsset(PooledSystem, true);
	}
	PooledNiagara->bAutoActivate = false;
	PooledNiagara->SetAutoDestroy(false);
	PooledNiagara->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	if (MaxSimTime > 0.f)
	{
		PooledNiagara->SetMaxSimTime(MaxSimTime);
	}
	PooledNiagara->RegisterComponent();
}

void UDFNiagaraPoolComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PooledNiagara)
	{
		PooledNiagara->DestroyComponent();
		PooledNiagara = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void UDFNiagaraPoolComponent::PlayPooledVFX()
{
	if (PooledNiagara)
	{
		if (PooledSystem && PooledNiagara->GetAsset() != PooledSystem)
		{
			PooledNiagara->SetAsset(PooledSystem, true);
		}
		PooledNiagara->Activate(true);
	}
}

void UDFNiagaraPoolComponent::StopPooledVFX()
{
	if (PooledNiagara)
	{
		PooledNiagara->Deactivate();
	}
}
