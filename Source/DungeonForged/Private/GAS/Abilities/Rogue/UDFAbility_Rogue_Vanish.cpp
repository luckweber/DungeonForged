// Source/DungeonForged/Private/GAS/Abilities/Rogue/UDFAbility_Rogue_Vanish.cpp
#include "GAS/Abilities/Rogue/UDFAbility_Rogue_Vanish.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbility.h"
#include "AIController.h"
#include "AI/DFAIKeys.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Characters/ADFPlayerState.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Buff_Rogue_Ambush.h"
#include "GAS/Effects/UGE_Rogue_Vanish_Stealth.h"
#include "GAS/UDFAttributeSet.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"

UDFAbility_Rogue_Vanish::UDFAbility_Rogue_Vanish()
{
	AbilityCost_Mana = 0.f;
	AbilityCost_Stamina = 50.f;
	BaseCooldown = 45.f;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UDFAbility_Rogue_Vanish::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Rogue_Vanish);
		AbilityTags.AddTag(FDFGameplayTags::Ability_Rogue);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
	}
}

void UDFAbility_Rogue_Vanish::UnbindAll()
{
	if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AbilityActivatedCallbacks.RemoveAll(this);
	}
	if (BoundAttr)
	{
		BoundAttr->OnHealthChanged.RemoveAll(this);
		BoundAttr = nullptr;
	}
}

void UDFAbility_Rogue_Vanish::OnHealthForVanish(float NewHealth, float /*MaxHealth*/)
{
	if (bTearingDown)
	{
		return;
	}
	if (NewHealth < LastHealthSnapshot - 0.25f)
	{
		BreakVanish(false);
	}
}

void UDFAbility_Rogue_Vanish::OnAnyAbility(UGameplayAbility* const A)
{
	if (bTearingDown || !A || A == this)
	{
		return;
	}
	const UGameplayAbility* const Def = A->GetClass()->GetDefaultObject<UGameplayAbility>();
	if (Def && Def->AbilityTags.HasTag(FDFGameplayTags::Ability_Attack))
	{
		BreakVanish(true);
	}
}

void UDFAbility_Rogue_Vanish::BreakVanish(const bool bFromAttack)
{
	if (bTearingDown)
	{
		return;
	}
	bTearingDown = true;
	UAbilitySystemComponent* const Src = GetAbilitySystemComponentFromActorInfo();
	if (Src)
	{
		if (VanishHandle.IsValid())
		{
			Src->RemoveActiveGameplayEffect(VanishHandle, 0);
			VanishHandle = FActiveGameplayEffectHandle();
		}
	}
	UnbindAll();
	RestoreMeshOpacity();
	if (bFromAttack && Src)
	{
		const FGameplayEffectContextHandle Cx = Src->MakeEffectContext();
		if (Cx.IsValid())
		{
			const FGameplayEffectSpecHandle S = Src->MakeOutgoingSpec(UGE_Buff_Rogue_Ambush::StaticClass(), 1.f, Cx);
			if (S.IsValid() && S.Data)
			{
				Src->ApplyGameplayEffectSpecToSelf(*S.Data);
			}
		}
	}
	if (ACharacter* C = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}

void UDFAbility_Rogue_Vanish::RestoreMeshOpacity()
{
	if (ACharacter* C = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (USkeletalMeshComponent* M = C->GetMesh())
		{
			for (int32 I = 0; I < M->GetNumMaterials(); ++I)
			{
				if (MeshOpacities.IsValidIndex(I) && MeshOpacities[I])
				{
					MeshOpacities[I]->SetScalarParameterValue(FName(TEXT("Opacity")), 1.f);
				}
			}
		}
	}
	MeshOpacities.Empty();
}

void UDFAbility_Rogue_Vanish::OnWaitFinished()
{
	if (!bTearingDown)
	{
		BreakVanish(false);
	}
}

void UDFAbility_Rogue_Vanish::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	(void)TriggerEventData;
	if (!ActorInfo)
	{
		EndAbility(Handle, nullptr, ActivationInfo, true, true);
		return;
	}
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo, nullptr))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	bTearingDown = false;
	VanishHandle = FActiveGameplayEffectHandle();
	MeshOpacities.Empty();
	UAbilitySystemComponent* const Src = GetAbilitySystemComponentFromActorInfo();
	ACharacter* const C = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	ADFPlayerState* const PS = C ? C->GetPlayerState<ADFPlayerState>() : nullptr;
	if (Src->GetOwner() && Src->GetOwner()->HasAuthority())
	{
		ApplyResourceCostsToOwner(Src);
	}
	if (PS && PS->AttributeSet)
	{
		BoundAttr = PS->AttributeSet;
		LastHealthSnapshot = BoundAttr->GetHealth();
		BoundAttr->OnHealthChanged.AddUObject(this, &UDFAbility_Rogue_Vanish::OnHealthForVanish);
	}
	if (Src)
	{
		Src->AbilityActivatedCallbacks.AddUObject(this, &UDFAbility_Rogue_Vanish::OnAnyAbility);
	}
	{
		const FGameplayEffectContextHandle Ctx = Src->MakeEffectContext();
		const FGameplayEffectSpecHandle S = Src->MakeOutgoingSpec(UGE_Rogue_Vanish_Stealth::StaticClass(), 1.f, Ctx);
		if (S.IsValid() && S.Data && FDFGameplayTags::Data_Duration.IsValid())
		{
			S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, 8.f);
			VanishHandle = Src->ApplyGameplayEffectSpecToSelf(*S.Data);
		}
	}
	if (VanishPuffVFX && C)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, VanishPuffVFX, C->GetActorLocation(), C->GetActorRotation(), FVector(1.f), true, true, ENCPoolMethod::AutoRelease, true);
	}
	if (C && C->GetMesh())
	{
		USkeletalMeshComponent* M = C->GetMesh();
		for (int32 I = 0; I < M->GetNumMaterials(); ++I)
		{
			UMaterialInstanceDynamic* D = M->CreateDynamicMaterialInstance(I);
			if (D)
			{
				D->SetScalarParameterValue(FName(TEXT("Opacity")), StealthMeshOpacity);
				MeshOpacities.Add(D);
			}
		}
	}
	{
		FGameplayEventData E;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			C, FDFGameplayTags::Event_Stealth_Entered, E);
	}
	if (C && C->GetWorld())
	{
		APawn* const PlayerP = Cast<APawn>(C);
		for (TActorIterator<AAIController> It(C->GetWorld()); It; ++It)
		{
			if (UBlackboardComponent* const BB = It->GetBlackboardComponent())
			{
				if (AActor* const Tgt = Cast<AActor>(BB->GetValueAsObject(DFAIKeys::TargetActor));
					Tgt == PlayerP)
				{
					BB->SetValueAsObject(DFAIKeys::TargetActor, nullptr);
					BB->SetValueAsBool(DFAIKeys::bCanSeeTarget, false);
				}
			}
		}
	}
	if (UAbilityTask_WaitDelay* D = UAbilityTask_WaitDelay::WaitDelay(this, 8.f))
	{
		D->OnFinish.AddDynamic(this, &UDFAbility_Rogue_Vanish::OnWaitFinished);
		D->ReadyForActivation();
	}
}

void UDFAbility_Rogue_Vanish::EndAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const bool bReplicateEndAbility, const bool bWasCancelled)
{
	UnbindAll();
	RestoreMeshOpacity();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
