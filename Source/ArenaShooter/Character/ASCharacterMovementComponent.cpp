// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ASCharacterMovementComponent.h"

#include "GameFramework/Character.h"

void UASCharacterMovementComponent::AddDampedImpulse(FVector Impulse, bool bSelfInflicted)
{
	if (!HasValidData() || Impulse.IsZero())
	{
		return;
	}

	FVector FinalImpulse = Impulse;
	if (Mass > UE_SMALL_NUMBER)
	{
		FinalImpulse /= Mass;
	}

	/* Horizontal pass: split the impulse into the part running along our current velocity and the part
	 * orthogonal to it, then progressively damp the former the faster we're already going.
	 * Standing still leaves XYDelta at zero, so the first explosion always lands undamped. */
	const float FinalImpulseZ = FinalImpulse.Z;
	FinalImpulse.Z = 0.f;

	const FVector PendingVelocity = Velocity + PendingImpulseToApply;
	const FVector PendingVelocityDir = PendingVelocity.GetSafeNormal();
	const FVector AdditiveImpulse = PendingVelocityDir * (PendingVelocityDir | FinalImpulse);
	const FVector OrthogonalImpulse = FinalImpulse - AdditiveImpulse;
	const float CurrentXYSpeed = PendingVelocity.Size2D();
	const float XYDelta = (PendingVelocity + AdditiveImpulse).Size2D() - CurrentXYSpeed;

	if (XYDelta > 0.f)
	{
		const float AboveDampFactor = (KnockbackDampingSpeed > UE_SMALL_NUMBER) ? (CurrentXYSpeed / KnockbackDampingSpeed) : 0.f;
		if (AboveDampFactor > 1.f)
		{
			FinalImpulse = AdditiveImpulse / AboveDampFactor + OrthogonalImpulse;
		}

		const float PctBelowRun = FMath::Clamp((MaxWalkSpeed - CurrentXYSpeed) / XYDelta, 0.f, 1.f);
		float PctBelowDamp = FMath::Clamp((KnockbackDampingSpeed - CurrentXYSpeed) / XYDelta, 0.f, 1.f);
		const float PctAboveDamp = FMath::Max(0.f, 1.f - PctBelowDamp);
		PctBelowDamp = FMath::Max(0.f, PctBelowDamp - PctBelowRun);

		FinalImpulse *= (PctBelowRun + PctBelowDamp + FMath::Max(0.5f, 1.f - PctAboveDamp) * PctAboveDamp);

		FVector FinalVelocityXY = PendingVelocity + FinalImpulse;
		FinalVelocityXY.Z = 0.f;
		if (FinalVelocityXY.Size() > MaxKnockbackHorizontalVelocity)
		{
			FinalImpulse = FinalVelocityXY.GetSafeNormal() * MaxKnockbackHorizontalVelocity - PendingVelocity;
		}
	}

	FinalImpulse.Z = FinalImpulseZ;

	// Vertical pass: self-inflicted impulses get a higher undamped ceiling, so your own rocket lifts you
	// further than someone else's does.
	const float DampingThreshold = bSelfInflicted ? MaxUndampedImpulseZ : MaxAdditiveKnockbackZ;
	if (FinalImpulse.Z > 0.f && (FinalImpulse.Z + PendingVelocity.Z) > DampingThreshold)
	{
		const float PctBelowBoost = FMath::Clamp((DampingThreshold - PendingVelocity.Z) / FinalImpulse.Z, 0.f, 1.f);
		FinalImpulse.Z *= (PctBelowBoost + (1.f - PctBelowBoost) * FMath::Max(0.5f, PctBelowBoost));
	}

	PendingImpulseToApply += FinalImpulse;

	ForceClientAdjustment();
}

void UASCharacterMovementComponent::ForceClientAdjustment()
{
	if (CharacterOwner && CharacterOwner->HasAuthority() && CharacterOwner->GetRemoteRole() == ROLE_AutonomousProxy)
	{
		if (FNetworkPredictionData_Server_Character* ServerData = GetPredictionData_Server_Character())
		{
			ServerData->bForceClientUpdate = true;
		}
	}
}
