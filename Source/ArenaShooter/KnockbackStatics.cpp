// Fill out your copyright notice in the Description page of Project Settings.


#include "KnockbackStatics.h"

#include "ASAbilityTypes.h"
#include "Components/ASCharacterMovementComponent.h"
#include "GameFramework/Character.h"

void UKnockbackStatics::ApplyKnockbackToActor(AActor* Target, const FVector& Origin, float Falloff, const FMomentumParams& Params, bool bIsSelfInflicted)
{
	ACharacter* Character = Cast<ACharacter>(Target);
	if (!Character || !Character->HasAuthority() || Falloff <= 0.f)
	{
		return;
	}

	UASCharacterMovementComponent* Movement = Cast<UASCharacterMovementComponent>(Character->GetCharacterMovement());
	if (!Movement)
	{
		return;
	}

	FVector Dir = (Character->GetActorLocation() - Origin).GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		Dir = FVector::UpVector;
	}

	FVector ResultMomentum = Dir * Params.BaseMomentum * Falloff;

	if (bIsSelfInflicted)
	{
		if (Params.bSelfMomentumBoostOnlyZ)
		{
			ResultMomentum.Z *= Params.SelfMomentumBoost;
		}
		else
		{
			ResultMomentum *= Params.SelfMomentumBoost;
		}
	}

	if (Params.bForceZMomentum && Movement->IsMovingOnGround())
	{
		ResultMomentum.Z = FMath::Max(ResultMomentum.Z, Params.ForceZMomentumPct * ResultMomentum.Size());
	}

	Movement->AddDampedImpulse(ResultMomentum, bIsSelfInflicted);
}
