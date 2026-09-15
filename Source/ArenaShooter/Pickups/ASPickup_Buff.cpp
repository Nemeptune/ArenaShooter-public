// Fill out your copyright notice in the Description page of Project Settings.


#include "ASPickup_Buff.h"

#include "AbilitySystemComponent.h"
#include "ASBuffDefinition.h"
#include "ASGameplayTags.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Net/UnrealNetwork.h"


AASPickup_Buff::AASPickup_Buff()
{
	IdleFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("IdleFX"));
	IdleFX->SetupAttachment(GetRootComponent());
	IdleFX->SetAutoActivate(false);
	
	RotatingMovement = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovement"));
	RotatingMovement->RotationRate = FRotator(0.f, 45.f, 0.f);
	
	PrimaryActorTick.bCanEverTick = false;
}

void AASPickup_Buff::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AASPickup_Buff, ActiveBuffIndex);
}

void AASPickup_Buff::BeginPlay()
{
	if (HasAuthority())
	{
		PickNewBuff();
	}
	
	Super::BeginPlay();
}

void AASPickup_Buff::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	ApplyFXParameters(IdleFX);

#if WITH_EDITOR
	// Editor-only: the component doesn't auto-activate, so preview it by hand.
	if (IdleFX && GetWorld() && !GetWorld()->IsGameWorld())
	{
		IdleFX->Activate(/*bReset=*/true);
	}
#endif
}

void AASPickup_Buff::ApplyFXParameters(UNiagaraComponent* Component) const
{
	const UASBuffDefinition* Buff = GetActiveBuff();
	if (!Component || !Buff)
	{
		return;
	}
	
	Component->SetVariableLinearColor(FName("User.Color"), Buff->Color);
	Component->SetVariableLinearColor(FName("User.Color Secondary"), Buff->ColorSecondary);
	
	if (Buff->PickupMesh)
	{
		Component->SetVariableStaticMesh(FName("User.Mesh"), Buff->PickupMesh);
	}
}

bool AASPickup_Buff::GiveTo(AActor* Actor, UAbilitySystemComponent* ASC)
{
	const UASBuffDefinition* Buff = GetActiveBuff();
	if (!ASC || !Buff || !Buff->BuffEffect)
	{
		return false;
	}
	
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);
	ASC->ApplyGameplayEffectToSelf(Buff->BuffEffect->GetDefaultObject<UGameplayEffect>(), 1.f, Context);
	
	return true;
}

const UASBuffDefinition* AASPickup_Buff::GetActiveBuff() const
{
	if (PossibleBuffs.IsValidIndex(ActiveBuffIndex))
	{
		return PossibleBuffs[ActiveBuffIndex];
	}
	
	return PossibleBuffs.IsEmpty() ? nullptr : PossibleBuffs[0].Get();
}

void AASPickup_Buff::PickNewBuff()
{
	if (PossibleBuffs.IsEmpty())
	{
		return;
	}
	
	if (bAvoidRepeatingLastBuff && PossibleBuffs.Num() > 1 && PossibleBuffs.IsValidIndex(ActiveBuffIndex))
	{
		int32 NewIndex = FMath::RandRange(0, PossibleBuffs.Num() - 2);
		if (NewIndex >= ActiveBuffIndex)
		{
			++NewIndex;
		}
		ActiveBuffIndex = NewIndex;
	}
	else
	{
		ActiveBuffIndex = FMath::RandRange(0, PossibleBuffs.Num() - 1);
	}
	
	OnRep_ActiveBuffIndex();
}


void AASPickup_Buff::OnActiveStateChanged(bool bPlayTransitionFX)
{
	if (HasAuthority() && bIsActive && bPlayTransitionFX)
	{
		PickNewBuff();
	}
	
	ApplyFXParameters(IdleFX);
		
	if (IdleFX)
	{
		if (bIsActive)
		{
			IdleFX->Activate(true);
		}
		else
		{
			IdleFX->Deactivate();
		}
	}
	
	if (!bPlayTransitionFX)
	{
		return;
	}
	
	if (UNiagaraSystem* FX = bIsActive ? RespawnFX : PickedUpFX)
	{
		UNiagaraComponent* Burst = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FX, GetActorLocation(), GetActorRotation(), FVector(1.f), true, false);
		
		if (Burst)
		{
			ApplyFXParameters(Burst);
			Burst->Activate(true);
		}
	}
}

bool AASPickup_Buff::CanGiveTo(UAbilitySystemComponent* ASC)
{
	if (!Super::CanGiveTo(ASC))
	{
		return false;
	}
	
	return !ASC->HasMatchingGameplayTag(FASGameplayTags::Buff);
}

void AASPickup_Buff::OnRep_ActiveBuffIndex()
{
	ApplyFXParameters(IdleFX);	
	
	if (IdleFX && bIsActive && IdleFX->IsActive())
	{
		IdleFX->Activate(true);
	}
}
